/*********************************************************************
*                             CineLogic                              *
*                      RTT Trace and Debug Bridge                    *
**********************************************************************
*                                                                    *
*                    (c) 2023 - 2026 CineLogic                       *
*                                                                    *
*                  Support: wenshuaisong@gmail.com                   *
*                                                                    *
**********************************************************************
----------------------------------------------------------------------
File    : CoreLogRecorderRuntime.c
Purpose : Core log recorder RTT drain runtime
---------------------------END-OF-HEADER------------------------------
*/

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "CoreLogRecorder_internal.h"
#include "RTTBridge.h"

/*********************************************************************
*
*       _Recorder_CheckSourceChannel()
*
*  Function description
*    Verify that a source RTT up-buffer exists before recording starts.
*/
static int _Recorder_CheckSourceChannel(CoreLogRecorder_SourceId_t source_id) {
    CoreLogRecorder_Source_t *source;

    if (source_id >= CORE_LOG_RECORDER_NUM_SOURCES) {
        return -1;
    }

    source = &_recorder_state.sources[source_id];
    if (!source->enabled) {
        return 0;
    }

    if (RTTBridge_CheckUpBufferChannel(source->channel) != 0) {
        fprintf(stderr,
                "[CoreLogRecorder] %s RTT up-buffer channel %u is not configured\n",
                source->name,
                source->channel);
        return -1;
    }

    return 0;
}


/*********************************************************************
*
*       _Recorder_CheckSourceChannels()
*
*  Function description
*    Verify all core RTT up-buffer channels before recorder threads start.
*/
static int _Recorder_CheckSourceChannels(void) {
    int      result;
    unsigned i;

    result = 0;
    for (i = 0u; i < CORE_LOG_RECORDER_NUM_SOURCES; i++) {
        if (!_Recorder_SourceIsEnabled((CoreLogRecorder_SourceId_t)i)) {
            continue;
        }
        if (_Recorder_CheckSourceChannel((CoreLogRecorder_SourceId_t)i) != 0) {
            result = -1;
        }
    }

    return result;
}


/*********************************************************************
*
*       _Recorder_GetRecoveryTimeout()
*
*  Function description
*    Return the RTT control block recovery timeout in milliseconds.
*/
static unsigned _Recorder_GetRecoveryTimeout(void) {
    const RTTBridge_State_t *bridge_state;

    bridge_state = RTTBridge_GetState();
    if (bridge_state == NULL) {
        return 0u;
    }

    return bridge_state->config.rtt_search_timeout_ms;
}


/*********************************************************************
*
*       _Recorder_WaitForSourceRecovery()
*
*  Function description
*    Wait until a source RTT up-buffer is valid again after target reset.
*/
static int _Recorder_WaitForSourceRecovery(CoreLogRecorder_SourceId_t source_id) {
    CoreLogRecorder_Source_t *source;
    unsigned                  retry_interval_ms;
    unsigned                  timeout_ms;
    unsigned                  elapsed_ms;
    unsigned                  sleep_ms;
    bool                      reported;

    if (source_id >= CORE_LOG_RECORDER_NUM_SOURCES) {
        _Recorder_SetFatalError();
        return -1;
    }

    source = &_recorder_state.sources[source_id];
    if (!source->enabled) {
        return -1;
    }

    retry_interval_ms = _Recorder_GetPollInterval();
    if (retry_interval_ms == 0u) {
        _Recorder_SetFatalError();
        return -1;
    }

    timeout_ms = _Recorder_GetRecoveryTimeout();
    elapsed_ms = 0u;
    reported = false;

    while (_Recorder_IsRunning() && RTTBridge_IsRunning()) {
        if (RTTBridge_CheckUpBufferChannel(source->channel) == 0) {
            if (reported) {
                fprintf(stderr,
                        "[CoreLogRecorder] %s RTT up-buffer channel %u recovered\n",
                        source->name,
                        source->channel);
            }
            return 0;
        }

        if (!reported) {
            fprintf(stderr,
                    "[CoreLogRecorder] %s RTT up-buffer channel %u unavailable, waiting for recovery\n",
                    source->name,
                    source->channel);
            reported = true;
        }

        if ((timeout_ms > 0u) && (elapsed_ms >= timeout_ms)) {
            fprintf(stderr,
                    "[CoreLogRecorder] %s RTT up-buffer channel %u did not recover within %u ms\n",
                    source->name,
                    source->channel,
                    timeout_ms);
            _Recorder_SetFatalError();
            return -1;
        }

        sleep_ms = retry_interval_ms;
        if (timeout_ms > 0u) {
            unsigned remaining_ms;

            remaining_ms = timeout_ms - elapsed_ms;
            if (sleep_ms > remaining_ms) {
                sleep_ms = remaining_ms;
            }
        }

        SYS_Sleep(sleep_ms);
        if (sleep_ms > (UINT_MAX - elapsed_ms)) {
            elapsed_ms = UINT_MAX;
        } else {
            elapsed_ms += sleep_ms;
        }
    }

    return -1;
}


/*********************************************************************
*
*       _Recorder_DrainSource()
*
*  Function description
*    Read one core RTT up-buffer and record every byte read.
*/
static void _Recorder_DrainSource(CoreLogRecorder_SourceId_t source_id) {
    char     buffer[CORE_LOG_RECORDER_BUFFER_SIZE];
    unsigned channel;
    int      num_bytes;

    channel = _recorder_state.sources[source_id].channel;
    if (!_Recorder_SourceIsEnabled(source_id)) {
        return;
    }

    while (_Recorder_IsRunning() && RTTBridge_IsRunning()) {
        num_bytes = RTTBridge_ReadUpBufferNoLock(channel, buffer, sizeof(buffer));
        if (num_bytes < 0) {
            if (_Recorder_WaitForSourceRecovery(source_id) != 0) {
                break;
            }
            continue;
        }
        if (num_bytes == 0) {
            SYS_Sleep(_Recorder_GetPollInterval());
            continue;
        }
        _Recorder_RecordBytes(source_id, buffer, (unsigned)num_bytes);
    }
}


/*********************************************************************
*
*       _Recorder_LinuxThread()
*
*  Function description
*    Thread entry for draining Linux core logs.
*
*  Parameters
*    arg  Unused thread context.
*/
static void _Recorder_LinuxThread(void *arg) {
    (void)arg;

    _Recorder_DrainSource(CORE_LOG_RECORDER_LINUX);
}


/*********************************************************************
*
*       _Recorder_RTOSThread()
*
*  Function description
*    Thread entry for draining RTOS core logs.
*
*  Parameters
*    arg  Unused thread context.
*/
static void _Recorder_RTOSThread(void *arg) {
    (void)arg;

    _Recorder_DrainSource(CORE_LOG_RECORDER_RTOS);
}


/*********************************************************************
*
*       CoreLogRecorder_Start()
*
*  Function description
*    Start recorder threads for enabled core log sources.
*
*  Return value
*    0   Success.
*   -1   Recorder is not initialized, has failed, or is already running.
*   -2   Failed to create a recorder thread.
*   -3   RTT source channel validation failed.
*/
int CoreLogRecorder_Start(void) {
    int result;

    if (!_recorder_state.initialized || _Recorder_HasFatalError()) {
        return -1;
    }
    if (_Recorder_IsRunning() ||
        _recorder_state.linux_thread_started ||
        _recorder_state.rtos_thread_started) {
        return -1;
    }
    if (_Recorder_CheckSourceChannels() != 0) {
        _Recorder_SetFatalError();
        return -3;
    }

    _Recorder_SetRunning(true);

    if (_Recorder_SourceIsEnabled(CORE_LOG_RECORDER_LINUX)) {
        result = SYS_createThread(_Recorder_LinuxThread, NULL, &_recorder_state.linux_thread);
        if (result < 0) {
            _Recorder_SetRunning(false);
            return -2;
        }
        _recorder_state.linux_thread_started = true;
    }

    if (_Recorder_SourceIsEnabled(CORE_LOG_RECORDER_RTOS)) {
        result = SYS_createThread(_Recorder_RTOSThread, NULL, &_recorder_state.rtos_thread);
        if (result < 0) {
            _Recorder_SetRunning(false);
            if (_recorder_state.linux_thread_started) {
                SYS_WaitThreadTerm(_recorder_state.linux_thread);
                _recorder_state.linux_thread_started = false;
            }
            return -2;
        }
        _recorder_state.rtos_thread_started = true;
    }

    return 0;
}


/*********************************************************************
*
*       CoreLogRecorder_Stop()
*
*  Function description
*    Stop recorder threads, flush pending file output, and report
*    recorder statistics.
*/
void CoreLogRecorder_Stop(void) {
    _Recorder_SetRunning(false);

    if (_recorder_state.linux_thread_started) {
        SYS_WaitThreadTerm(_recorder_state.linux_thread);
        _recorder_state.linux_thread_started = false;
    }
    if (_recorder_state.rtos_thread_started) {
        SYS_WaitThreadTerm(_recorder_state.rtos_thread);
        _recorder_state.rtos_thread_started = false;
    }

    _Recorder_FlushAllSources();
    _Recorder_ReportStats();
    _Recorder_CloseFiles();
}
