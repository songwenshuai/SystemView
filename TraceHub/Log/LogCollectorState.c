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
*                                                                    *
*       CineLogic TraceHub * RTT trace and debug bridge              *
*                                                                    *
**********************************************************************
*                                                                    *
* All rights reserved.                                               *
*                                                                    *
* CineLogic strongly recommends to not make any changes              *
* to or modify the source code of this software in order to stay     *
* compatible with the SharedMem and RTT data path.                   *
*                                                                    *
* Redistribution and use in source and binary forms, with or         *
* without modification, are permitted provided that the following    *
* condition is met:                                                  *
*                                                                    *
* o Redistributions of source code must retain the above copyright   *
*   notice, this condition and the following disclaimer.             *
*                                                                    *
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND             *
* CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,        *
* INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF           *
* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE           *
* DISCLAIMED. IN NO EVENT SHALL CINELOGIC BE LIABLE FOR              *
* ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR           *
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT  *
* OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;    *
* OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF      *
* LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT          *
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE  *
* USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH   *
* DAMAGE.                                                            *
*                                                                    *
**********************************************************************
----------------------------------------------------------------------
File    : LogCollectorState.c
Purpose : LogCollector shared state management
---------------------------END-OF-HEADER------------------------------
*/

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "LogCollector_internal.h"
#include "CoreLogRecorder.h"

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

static LogCollector_State_t _collector_state;

/*********************************************************************
*
*       Static functions
*
**********************************************************************
*/

/*********************************************************************
*
*       _Collector_GetSourceName()
*
*  Function description
*    Return a printable source name.
*/
static const char *_Collector_GetSourceName(LogSource_t source) {
    return (source == LOG_SOURCE_LINUX) ? "Linux" : "RTOS";
}

/*********************************************************************
*
*       _Collector_FreePendingBuffers()
*
*  Function description
*    Release pending untimestamped buffers owned by the collector.
*/
static void _Collector_FreePendingBuffers(void) {
    LogCollector_SourceState_t *linux_source;
    LogCollector_SourceState_t *rtos_source;

    linux_source = &_collector_state.sources[LOG_SOURCE_LINUX];
    rtos_source = &_collector_state.sources[LOG_SOURCE_RTOS];

    free(linux_source->pending_untimed);
    free(rtos_source->pending_untimed);
    linux_source->pending_untimed = NULL;
    rtos_source->pending_untimed = NULL;
    linux_source->pending_untimed_size = 0u;
    rtos_source->pending_untimed_size = 0u;
    linux_source->pending_untimed_len = 0u;
    rtos_source->pending_untimed_len = 0u;
    _collector_state.pending_untimed_size = 0u;
}

/*********************************************************************
*
*       _Collector_AllocatePendingBuffers()
*
*  Function description
*    Allocate per-source pending untimestamped buffers.
*/
static int _Collector_AllocatePendingBuffers(size_t size) {
    LogCollector_SourceState_t *linux_source;
    LogCollector_SourceState_t *rtos_source;

    if (size < LOG_COLLECTOR_MIN_PENDING_UNTIMED_SIZE) {
        return -1;
    }

    linux_source = &_collector_state.sources[LOG_SOURCE_LINUX];
    rtos_source = &_collector_state.sources[LOG_SOURCE_RTOS];

    linux_source->pending_untimed = (char *)malloc(size);
    if (linux_source->pending_untimed == NULL) {
        return -1;
    }

    rtos_source->pending_untimed = (char *)malloc(size);
    if (rtos_source->pending_untimed == NULL) {
        _Collector_FreePendingBuffers();
        return -1;
    }

    _collector_state.pending_untimed_size = size;
    linux_source->pending_untimed_size = size;
    rtos_source->pending_untimed_size = size;
    linux_source->pending_untimed[0] = '\0';
    rtos_source->pending_untimed[0] = '\0';
    return 0;
}

/*********************************************************************
*
*       _Collector_NextTimestamp()
*
*  Function description
*    Return the next representable timestamp after the supplied value.
*/
static uint64_t _Collector_NextTimestamp(uint64_t timestamp) {
    if (timestamp == UINT64_MAX) {
        return UINT64_MAX;
    }
    return timestamp + 1u;
}

/*********************************************************************
*
*       _Collector_NormalizeFatalResult()
*
*  Function description
*    Convert any terminal collector failure into the public poll result.
*/
static int _Collector_NormalizeFatalResult(int result) {
    if (result == LOG_COLLECT_RESULT_STOP ||
        result == LOG_COLLECT_RESULT_PENDING_OVERFLOW ||
        result == LOG_COLLECT_RESULT_INVALID_RTT) {
        return result;
    }
    return LOG_COLLECT_RESULT_INVALID_RTT;
}

/*********************************************************************
*
*       Public functions
*
**********************************************************************
*/

/*********************************************************************
*
*       LogCollectorState_Get()
*
*  Function description
*    Return the singleton collector state.
*/
LogCollector_State_t *LogCollectorState_Get(void) {
    return &_collector_state;
}

/*********************************************************************
*
*       LogCollectorState_Init()
*
*  Function description
*    Initialize collector state and owned synchronization resources.
*/
int LogCollectorState_Init(const LogCollector_Config_t *config) {
    int    result;
    size_t pending_untimed_size;

    if (config == NULL) {
        return -1;
    }
    if (LogCollectorState_IsRunning()) {
        return -1;
    }

    memset(&_collector_state, 0, sizeof(_collector_state));
    memcpy(&_collector_state.config, config, sizeof(LogCollector_Config_t));
    LogCollectorSource_Init(&_collector_state.sources[LOG_SOURCE_LINUX],
                            LOG_SOURCE_LINUX,
                            config->linux_channel);
    LogCollectorSource_Init(&_collector_state.sources[LOG_SOURCE_RTOS],
                            LOG_SOURCE_RTOS,
                            config->rtos_channel);

    pending_untimed_size = LOG_COLLECTOR_DEFAULT_PENDING_UNTIMED_SIZE;
    if (pending_untimed_size < LOG_COLLECTOR_MIN_PENDING_UNTIMED_SIZE) {
        return -1;
    }
    if (_Collector_AllocatePendingBuffers(pending_untimed_size) != 0) {
        LogCollectorState_ResetStorage();
        return -1;
    }

    result = SYS_MutexInit(&_collector_state.lock);
    if (result != 0) {
        _Collector_FreePendingBuffers();
        LogCollectorState_ResetStorage();
        return -1;
    }
    _collector_state.lock_initialized = true;

    result = SYS_MutexInit(&_collector_state.emit_lock);
    if (result != 0) {
        SYS_MutexDestroy(&_collector_state.lock);
        _collector_state.lock_initialized = false;
        _Collector_FreePendingBuffers();
        LogCollectorState_ResetStorage();
        return -1;
    }
    _collector_state.emit_lock_initialized = true;
    _collector_state.initialized = true;
    return 0;
}

/*********************************************************************
*
*       LogCollectorState_Destroy()
*
*  Function description
*    Release collector state resources and reset storage.
*/
void LogCollectorState_Destroy(void) {
    LogCollectorState_UnregisterConsumers();
    if (_collector_state.emit_lock_initialized) {
        SYS_MutexDestroy(&_collector_state.emit_lock);
        _collector_state.emit_lock_initialized = false;
    }
    if (_collector_state.lock_initialized) {
        SYS_MutexDestroy(&_collector_state.lock);
        _collector_state.lock_initialized = false;
    }
    _Collector_FreePendingBuffers();
    LogCollectorState_ResetStorage();
}

/*********************************************************************
*
*       LogCollectorState_ResetStorage()
*
*  Function description
*    Free pending buffers and clear the collector state.
*/
void LogCollectorState_ResetStorage(void) {
    _Collector_FreePendingBuffers();
    memset(&_collector_state, 0, sizeof(_collector_state));
}

/*********************************************************************
*
*       LogCollectorState_ResetSourcesForRun()
*
*  Function description
*    Reset per-source line, timestamp, pending, and sequence state.
*/
void LogCollectorState_ResetSourcesForRun(void) {
    LogCollectorSource_ResetForRun(&_collector_state.sources[LOG_SOURCE_LINUX]);
    LogCollectorSource_ResetForRun(&_collector_state.sources[LOG_SOURCE_RTOS]);
    _collector_state.global_last_timestamp = 0u;
    _collector_state.global_last_timestamp_valid = false;
}

/*********************************************************************
*
*       LogCollectorState_AnyThreadStarted()
*
*  Function description
*    Return whether any source collection thread is started.
*/
bool LogCollectorState_AnyThreadStarted(void) {
    return _collector_state.sources[LOG_SOURCE_LINUX].thread_started ||
           _collector_state.sources[LOG_SOURCE_RTOS].thread_started;
}

/*********************************************************************
*
*       LogCollectorState_IsRunning()
*
*  Function description
*    Read the collector running state through the module lock.
*/
bool LogCollectorState_IsRunning(void) {
    bool running;

    if (!_collector_state.lock_initialized) {
        return _collector_state.running;
    }

    SYS_MutexLock(&_collector_state.lock);
    running = _collector_state.running;
    SYS_MutexUnlock(&_collector_state.lock);
    return running;
}

/*********************************************************************
*
*       LogCollectorState_SetRunning()
*
*  Function description
*    Update the collector running state through the module lock.
*/
void LogCollectorState_SetRunning(bool running) {
    if (!_collector_state.lock_initialized) {
        _collector_state.running = running;
        return;
    }

    SYS_MutexLock(&_collector_state.lock);
    _collector_state.running = running;
    SYS_MutexUnlock(&_collector_state.lock);
}

/*********************************************************************
*
*       LogCollectorState_LockEmission()
*
*  Function description
*    Serialize timestamp assignment and entry delivery across source threads.
*/
void LogCollectorState_LockEmission(void) {
    if (_collector_state.emit_lock_initialized) {
        SYS_MutexLock(&_collector_state.emit_lock);
    }
}

/*********************************************************************
*
*       LogCollectorState_UnlockEmission()
*
*  Function description
*    Release the collector emission serialization lock.
*/
void LogCollectorState_UnlockEmission(void) {
    if (_collector_state.emit_lock_initialized) {
        SYS_MutexUnlock(&_collector_state.emit_lock);
    }
}

/*********************************************************************
*
*       LogCollectorState_RecordObservedTimestamp()
*
*  Function description
*    Record the greatest explicit or inferred timestamp observed by the
*    collector.
*/
void LogCollectorState_RecordObservedTimestamp(uint64_t timestamp) {
    if (!_collector_state.lock_initialized) {
        if (!_collector_state.global_last_timestamp_valid ||
            timestamp > _collector_state.global_last_timestamp) {
            _collector_state.global_last_timestamp = timestamp;
            _collector_state.global_last_timestamp_valid = true;
        }
        return;
    }

    SYS_MutexLock(&_collector_state.lock);
    if (!_collector_state.global_last_timestamp_valid ||
        timestamp > _collector_state.global_last_timestamp) {
        _collector_state.global_last_timestamp = timestamp;
        _collector_state.global_last_timestamp_valid = true;
    }
    SYS_MutexUnlock(&_collector_state.lock);
}

/*********************************************************************
*
*       LogCollectorState_ReserveUntimedTimestamp()
*
*  Function description
*    Reserve a deterministic timestamp for a line that has no explicit
*    timestamp.
*/
uint64_t LogCollectorState_ReserveUntimedTimestamp(uint64_t requested_timestamp) {
    uint64_t timestamp;
    bool     valid;

    if (!_collector_state.lock_initialized) {
        valid = _collector_state.global_last_timestamp_valid;
        timestamp = _collector_state.global_last_timestamp;
        if (valid && requested_timestamp < timestamp) {
            requested_timestamp = _Collector_NextTimestamp(timestamp);
        }
        if (!valid || requested_timestamp > _collector_state.global_last_timestamp) {
            _collector_state.global_last_timestamp = requested_timestamp;
            _collector_state.global_last_timestamp_valid = true;
        }
        return requested_timestamp;
    }

    SYS_MutexLock(&_collector_state.lock);
    valid = _collector_state.global_last_timestamp_valid;
    timestamp = _collector_state.global_last_timestamp;
    if (valid && requested_timestamp < timestamp) {
        requested_timestamp = _Collector_NextTimestamp(timestamp);
    }
    if (!valid || requested_timestamp > _collector_state.global_last_timestamp) {
        _collector_state.global_last_timestamp = requested_timestamp;
        _collector_state.global_last_timestamp_valid = true;
    }
    SYS_MutexUnlock(&_collector_state.lock);
    return requested_timestamp;
}

/*********************************************************************
*
*       LogCollectorState_HasFatalError()
*
*  Function description
*    Read the collector fatal state through the module lock.
*/
bool LogCollectorState_HasFatalError(void) {
    bool fatal_error;

    if (!_collector_state.lock_initialized) {
        return _collector_state.fatal_error;
    }

    SYS_MutexLock(&_collector_state.lock);
    fatal_error = _collector_state.fatal_error;
    SYS_MutexUnlock(&_collector_state.lock);
    return fatal_error;
}

/*********************************************************************
*
*       LogCollectorState_GetFatalResult()
*
*  Function description
*    Read the fatal result through the module lock.
*/
int LogCollectorState_GetFatalResult(void) {
    int result;

    if (!_collector_state.lock_initialized) {
        if (!_collector_state.fatal_error) {
            return 0;
        }
        return _Collector_NormalizeFatalResult(_collector_state.fatal_result);
    }

    SYS_MutexLock(&_collector_state.lock);
    if (_collector_state.fatal_error) {
        result = _Collector_NormalizeFatalResult(_collector_state.fatal_result);
    } else {
        result = 0;
    }
    SYS_MutexUnlock(&_collector_state.lock);
    return result;
}

/*********************************************************************
*
*       LogCollectorState_SetFatalErrorResult()
*
*  Function description
*    Mark the collector as failed with a stable terminal result.
*/
void LogCollectorState_SetFatalErrorResult(int result) {
    result = _Collector_NormalizeFatalResult(result);

    if (!_collector_state.lock_initialized) {
        if (!_collector_state.fatal_error) {
            _collector_state.fatal_result = result;
        }
        _collector_state.fatal_error = true;
        _collector_state.running = false;
        return;
    }

    SYS_MutexLock(&_collector_state.lock);
    if (!_collector_state.fatal_error) {
        _collector_state.fatal_result = result;
    }
    _collector_state.fatal_error = true;
    _collector_state.running = false;
    SYS_MutexUnlock(&_collector_state.lock);
}

/*********************************************************************
*
*       LogCollectorState_SetFatalError()
*
*  Function description
*    Mark the collector as failed and stop background collection.
*/
void LogCollectorState_SetFatalError(void) {
    LogCollectorState_SetFatalErrorResult(LOG_COLLECT_RESULT_INVALID_RTT);
}

/*********************************************************************
*
*       LogCollectorState_EnterFatalStateWithResult()
*
*  Function description
*    Mark collector as failed and report whether this is the first fatal
*    transition.
*/
bool LogCollectorState_EnterFatalStateWithResult(int result) {
    bool should_report;

    result = _Collector_NormalizeFatalResult(result);

    if (!_collector_state.lock_initialized) {
        should_report = !_collector_state.fatal_error;
        if (should_report) {
            _collector_state.fatal_result = result;
        }
        _collector_state.fatal_error = true;
        _collector_state.running = false;
        return should_report;
    }

    SYS_MutexLock(&_collector_state.lock);
    should_report = !_collector_state.fatal_error;
    if (should_report) {
        _collector_state.fatal_result = result;
    }
    _collector_state.fatal_error = true;
    _collector_state.running = false;
    SYS_MutexUnlock(&_collector_state.lock);
    return should_report;
}

/*********************************************************************
*
*       LogCollectorState_EnterFatalState()
*
*  Function description
*    Mark collector as failed with the default fatal result.
*/
bool LogCollectorState_EnterFatalState(void) {
    return LogCollectorState_EnterFatalStateWithResult(LOG_COLLECT_RESULT_INVALID_RTT);
}

/*********************************************************************
*
*       LogCollectorState_ReportRTTError()
*
*  Function description
*    Report an unrecoverable collector RTT or recorder error.
*/
void LogCollectorState_ReportRTTError(LogSource_t source, const char *operation) {
    bool should_report;

    if (operation == NULL) {
        operation = "access";
    }

    should_report = LogCollectorState_EnterFatalState();
    if (should_report) {
        fprintf(stderr,
                "[LogCollector] %s: fatal %s failure\n",
                _Collector_GetSourceName(source),
                operation);
    }
}

/*********************************************************************
*
*       LogCollectorState_ReportCallbackStop()
*
*  Function description
*    Report an unrecoverable callback stop request from background
*    collection.
*/
void LogCollectorState_ReportCallbackStop(LogSource_t source) {
    if (LogCollectorState_EnterFatalStateWithResult(LOG_COLLECT_RESULT_STOP)) {
        fprintf(stderr,
                "[LogCollector] %s: output callback requested stop\n",
                _Collector_GetSourceName(source));
    }
}

/*********************************************************************
*
*       LogCollectorState_ReportFlushError()
*
*  Function description
*    Report a pending-line flush failure during shutdown.
*/
void LogCollectorState_ReportFlushError(int result) {
    const char *reason;

    if (result >= 0 || result == LOG_COLLECT_RESULT_PENDING_OVERFLOW) {
        return;
    }

    if (result == LOG_COLLECT_RESULT_STOP) {
        reason = "output callback requested stop while flushing pending lines";
    } else if (result == LOG_COLLECT_RESULT_INVALID_RTT) {
        reason = "invalid pending line state";
    } else {
        reason = "unexpected pending line flush failure";
    }

    if (LogCollectorState_EnterFatalStateWithResult(result)) {
        fprintf(stderr, "[LogCollector] fatal shutdown flush failure: %s\n", reason);
    }
}

/*********************************************************************
*
*       LogCollectorState_EnsureConsumersRegistered()
*
*  Function description
*    Ensure LogCollector owns the consumer registrations for both core
*    channels.
*/
int LogCollectorState_EnsureConsumersRegistered(void) {
    int                         result;
    LogCollector_SourceState_t *linux_source;
    LogCollector_SourceState_t *rtos_source;
    bool                        linux_registered_here;

    linux_source = &_collector_state.sources[LOG_SOURCE_LINUX];
    rtos_source = &_collector_state.sources[LOG_SOURCE_RTOS];

    linux_registered_here = false;
    if (!linux_source->consumer_registered) {
        result = CoreLogRecorder_RegisterConsumer(linux_source->channel);
        if (result != 0) {
            return -1;
        }
        linux_source->consumer_registered = true;
        linux_registered_here = true;
    }

    if (!rtos_source->consumer_registered) {
        result = CoreLogRecorder_RegisterConsumer(rtos_source->channel);
        if (result != 0) {
            if (linux_registered_here) {
                CoreLogRecorder_UnregisterConsumer(linux_source->channel);
                linux_source->consumer_registered = false;
            }
            return -1;
        }
        rtos_source->consumer_registered = true;
    }

    return 0;
}

/*********************************************************************
*
*       LogCollectorState_UnregisterConsumers()
*
*  Function description
*    Release any core recorder consumer registrations owned by LogCollector.
*/
void LogCollectorState_UnregisterConsumers(void) {
    LogCollector_SourceState_t *linux_source;
    LogCollector_SourceState_t *rtos_source;

    linux_source = &_collector_state.sources[LOG_SOURCE_LINUX];
    rtos_source = &_collector_state.sources[LOG_SOURCE_RTOS];

    if (rtos_source->consumer_registered) {
        CoreLogRecorder_UnregisterConsumer(rtos_source->channel);
        rtos_source->consumer_registered = false;
    }
    if (linux_source->consumer_registered) {
        CoreLogRecorder_UnregisterConsumer(linux_source->channel);
        linux_source->consumer_registered = false;
    }
}

/*********************************************************************
*
*       LogCollectorState_StopCollectionWithResult()
*
*  Function description
*    Release collector consumers and return a terminal collection result.
*/
int LogCollectorState_StopCollectionWithResult(int result) {
    if (result == LOG_COLLECT_RESULT_INVALID_RTT ||
        result == LOG_COLLECT_RESULT_PENDING_OVERFLOW) {
        LogCollectorState_SetFatalErrorResult(result);
    }
    LogCollectorState_UnregisterConsumers();
    return result;
}

/*********************************************************************
*
*       LogCollectorState_RecordPendingOverflow()
*
*  Function description
*    Report that leading untimestamped lines exceeded the pending buffer.
*/
void LogCollectorState_RecordPendingOverflow(LogSource_t source, size_t limit) {
    fprintf(stderr,
            "[LogCollector] %s: untimestamped startup log exceeds %zu pending bytes; stopping to avoid reordering\n",
            _Collector_GetSourceName(source),
            limit);
    LogCollectorState_SetFatalErrorResult(LOG_COLLECT_RESULT_PENDING_OVERFLOW);
}

/*********************************************************************
*
*       LogCollector_Init()
*
*  Function description
*    Initialize the log collector with the specified configuration.
*
*  Parameters
*    config  Pointer to configuration structure
*
*  Return value
*    0   Success
*   -1   Invalid configuration
*/
int LogCollector_Init(LogCollector_Config_t *config) {
    LogCollector_State_t *state;

    if (config == NULL) {
        return -1;
    }
    if (LogCollectorState_IsRunning()) {
        return -1;
    }

    state = LogCollectorState_Get();
    if (state->initialized) {
        LogCollector_Cleanup();
    }

    return LogCollectorState_Init(config);
}

/*********************************************************************
*
*       LogCollector_Cleanup()
*
*  Function description
*    Cleanup log collector resources.
*/
void LogCollector_Cleanup(void) {
    LogCollector_State_t *state;
    unsigned              i;

    state = LogCollectorState_Get();
    if (LogCollectorState_IsRunning() || LogCollectorState_AnyThreadStarted()) {
        LogCollector_Stop();
    }
    for (i = 0u; i < LOG_SOURCE_MAX; i++) {
        LogCollectorSource_ReportUnflushedUntimedLines(&state->sources[i]);
    }
    LogCollectorState_Destroy();
}

/*********************************************************************
*
*       LogCollector_IsRunning()
*
*  Function description
*    Check whether log collection threads are running.
*
*  Return value
*    true   Collector is running.
*    false  Collector is stopped.
*/
bool LogCollector_IsRunning(void) {
    return LogCollectorState_IsRunning();
}

/*********************************************************************
*
*       LogCollector_HasFatalError()
*
*  Function description
*    Check whether the collector has entered a fatal error state.
*
*  Return value
*    true   Fatal error occurred.
*    false  No fatal error recorded.
*/
bool LogCollector_HasFatalError(void) {
    return LogCollectorState_HasFatalError();
}

/*************************** End of file ****************************/
