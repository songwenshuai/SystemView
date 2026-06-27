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
File    : CoreLogRecorderState.c
Purpose : Core log recorder shared state management
---------------------------END-OF-HEADER------------------------------
*/

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "CoreLogRecorder_internal.h"
#include "RTTBridge.h"

CoreLogRecorder_State_t _recorder_state;

/*********************************************************************
*
*       _Recorder_IsRunning()
*
*  Function description
*    Read running state through the module lock.
*/
bool _Recorder_IsRunning(void) {
    bool running;

    if (!_recorder_state.lock_initialized) {
        return _recorder_state.running;
    }

    SYS_MutexLock(&_recorder_state.lock);
    running = _recorder_state.running;
    SYS_MutexUnlock(&_recorder_state.lock);
    return running;
}


/*********************************************************************
*
*       _Recorder_SetRunning()
*
*  Function description
*    Update running state through the module lock.
*/
void _Recorder_SetRunning(bool running) {
    if (!_recorder_state.lock_initialized) {
        _recorder_state.running = running;
        return;
    }

    SYS_MutexLock(&_recorder_state.lock);
    _recorder_state.running = running;
    SYS_MutexUnlock(&_recorder_state.lock);
}


/*********************************************************************
*
*       _Recorder_SetFatalError()
*
*  Function description
*    Mark recorder fatal state and stop recorder threads.
*/
void _Recorder_SetFatalError(void) {
    if (!_recorder_state.lock_initialized) {
        _recorder_state.fatal_error = true;
        _recorder_state.running = false;
        return;
    }

    SYS_MutexLock(&_recorder_state.lock);
    _recorder_state.fatal_error = true;
    _recorder_state.running = false;
    SYS_MutexUnlock(&_recorder_state.lock);
}


/*********************************************************************
*
*       _Recorder_HasFatalError()
*
*  Function description
*    Read fatal error state through the module lock.
*/
bool _Recorder_HasFatalError(void) {
    bool fatal_error;

    if (!_recorder_state.lock_initialized) {
        return _recorder_state.fatal_error;
    }

    SYS_MutexLock(&_recorder_state.lock);
    fatal_error = _recorder_state.fatal_error;
    SYS_MutexUnlock(&_recorder_state.lock);
    return fatal_error;
}


/*********************************************************************
*
*       _Recorder_GetPollInterval()
*
*  Function description
*    Return the configured poll interval or a documented default.
*/
unsigned _Recorder_GetPollInterval(void) {
    if (_recorder_state.config.poll_interval_ms == 0u) {
        return RTT_BRIDGE_DEFAULT_POLL_INTERVAL_MS;
    }

    return _recorder_state.config.poll_interval_ms;
}


/*********************************************************************
*
*       CoreLogRecorder_Init()
*
*  Function description
*    Initialize the core log recorder and prepare enabled source output files.
*
*  Parameters
*    config  Recorder configuration.
*
*  Return value
*    0   Success.
*   -1   Invalid configuration or recorder state.
*   -2   Failed to initialize an enabled source.
*/
int CoreLogRecorder_Init(const CoreLogRecorder_Config_t *config) {
    const char *linux_prefix;
    const char *rtos_prefix;

    if (config == NULL) {
        return -1;
    }
    if (!config->linux_enabled && !config->rtos_enabled) {
        return -1;
    }
    if (config->linux_enabled &&
        config->rtos_enabled &&
        config->linux_channel == config->rtos_channel) {
        return -1;
    }
    if (_Recorder_IsRunning()) {
        return -1;
    }
    if (_recorder_state.initialized) {
        CoreLogRecorder_Cleanup();
    }
    if (CORE_LOG_RECORDER_DEFAULT_QUEUE_SIZE < CORE_LOG_RECORDER_MIN_QUEUE_SIZE) {
        return -1;
    }

    memset(&_recorder_state, 0, sizeof(_recorder_state));
    memcpy(&_recorder_state.config, config, sizeof(CoreLogRecorder_Config_t));

    if (SYS_MutexInit(&_recorder_state.lock) != 0) {
        return -1;
    }
    _recorder_state.lock_initialized = true;

    linux_prefix = (config->linux_prefix != NULL) ? config->linux_prefix : "linux";
    rtos_prefix  = (config->rtos_prefix  != NULL) ? config->rtos_prefix  : "rtos";

    if (_Recorder_InitSource(CORE_LOG_RECORDER_LINUX,
                             config->linux_channel,
                             config->linux_enabled,
                             "Linux",
                             linux_prefix) != 0) {
        CoreLogRecorder_Cleanup();
        return -2;
    }
    if (_Recorder_InitSource(CORE_LOG_RECORDER_RTOS,
                             config->rtos_channel,
                             config->rtos_enabled,
                             "RTOS",
                             rtos_prefix) != 0) {
        CoreLogRecorder_Cleanup();
        return -2;
    }

    _recorder_state.initialized = true;
    return 0;
}


/*********************************************************************
*
*       CoreLogRecorder_Cleanup()
*
*  Function description
*    Stop recording when needed and release recorder resources.
*/
void CoreLogRecorder_Cleanup(void) {
    if (_Recorder_IsRunning() ||
        _recorder_state.linux_thread_started ||
        _recorder_state.rtos_thread_started) {
        CoreLogRecorder_Stop();
    }

    _Recorder_CloseFiles();
    _Recorder_DestroySourceLocks();
    _Recorder_FreeSourceQueues();

    if (_recorder_state.lock_initialized) {
        SYS_MutexDestroy(&_recorder_state.lock);
        _recorder_state.lock_initialized = false;
    }

    memset(&_recorder_state, 0, sizeof(_recorder_state));
}


/*********************************************************************
*
*       CoreLogRecorder_HasFatalError()
*
*  Function description
*    Check whether the recorder has entered a fatal error state.
*
*  Return value
*    true   Fatal error occurred.
*    false  No fatal error recorded.
*/
bool CoreLogRecorder_HasFatalError(void) {
    return _Recorder_HasFatalError();
}


/*********************************************************************
*
*       CoreLogRecorder_IsRunning()
*
*  Function description
*    Check whether core log recorder threads are running.
*
*  Return value
*    true   Recorder is running.
*    false  Recorder is stopped.
*/
bool CoreLogRecorder_IsRunning(void) {
    return _Recorder_IsRunning();
}
