/*********************************************************************
*                             CineLogic                              *
*                      RTT Trace and Debug Bridge                    *
**********************************************************************
----------------------------------------------------------------------
File    : LogCollectorStorage.c
Purpose : LogCollector shared state management
---------------------------END-OF-HEADER------------------------------
*/

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include <stdlib.h>
#include <string.h>

#include "LogCollector_internal.h"

/*********************************************************************
*
*       Shared state
*
**********************************************************************
*/

LogCollector_State_t _collector_state;

/*********************************************************************
*
*       Static functions
*
**********************************************************************
*/

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
*       LogCollectorState_GetSourceName()
*
*  Function description
*    Return a printable source name.
*/
const char *LogCollectorState_GetSourceName(LogSource_t source) {
    return (source == LOG_SOURCE_LINUX) ? "Linux" : "RTOS";
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

/*************************** End of file ****************************/
