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
File    : LogMergerApi.c
Purpose : Log merger public API orchestration
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

#include "LogMerger_internal.h"

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

LogMerger_State_t _merger_state;

/*********************************************************************
*
*       Public functions
*
**********************************************************************
*/

/*********************************************************************
*
*       LogMerger_Init()
*
*  Function description
*    Initialize the log merger with the specified configuration.
*
*  Parameters
*    config  Pointer to configuration structure
*
*  Return value
*    0   Success
*   -1   Invalid configuration or already initialized
*   -2   Failed to allocate buffer
*   -3   Failed to create log file
*   -4   Failed to initialize mutex
*/
int LogMerger_Init(LogMerger_Config_t *config) {
    unsigned i;
    bool     has_required_source;
    //
    // Validate configuration
    //
    if (config == NULL) {
        return -1;
    }
    if (_merger_state.initialized || _merger_state.operation_mutex_initialized ||
        _merger_state.mutex_initialized || (_merger_state.buffer != NULL) ||
        (_merger_state.log_file != NULL)) {
        return -1;
    }
    if (config->buffer_size == 0) {
        return -1;
    }
    if (LOG_MERGER_DEFAULT_FLUSH_THRESHOLD == 0u ||
        LOG_MERGER_DEFAULT_FLUSH_THRESHOLD > config->buffer_size) {
        return -1;
    }
    has_required_source = false;
    for (i = 0; i < LOG_SOURCE_MAX; i++) {
        if (config->required_source[i]) {
            has_required_source = true;
            break;
        }
    }
    if (!has_required_source) {
        return -1;
    }
    //
    // Initialize state
    //
    memset(&_merger_state, 0, sizeof(_merger_state));
    memcpy(&_merger_state.config, config, sizeof(LogMerger_Config_t));
    _merger_state.flush_threshold = LOG_MERGER_DEFAULT_FLUSH_THRESHOLD;
    _merger_state.flush_timeout_ms = LOG_MERGER_DEFAULT_FLUSH_TIMEOUT_MS;
    _Merger_InitLogFileState();
    //
    // Initialize mutexes for thread safety
    //
    if (SYS_MutexInit(&_merger_state.operation_mutex) != 0) {
        return -4;
    }
    _merger_state.operation_mutex_initialized = true;

    if (SYS_MutexInit(&_merger_state.mutex) != 0) {
        SYS_MutexDestroy(&_merger_state.operation_mutex);
        _merger_state.operation_mutex_initialized = false;
        return -4;
    }
    _merger_state.mutex_initialized = true;
    //
    // Allocate buffer
    //
    _merger_state.buffer_size = config->buffer_size;
    _merger_state.buffer = (LogEntry_t **)malloc(
        sizeof(LogEntry_t *) * _merger_state.buffer_size
    );
    if (_merger_state.buffer == NULL) {
        SYS_MutexDestroy(&_merger_state.mutex);
        _merger_state.mutex_initialized = false;
        SYS_MutexDestroy(&_merger_state.operation_mutex);
        _merger_state.operation_mutex_initialized = false;
        return -2;
    }
    //
    // Create log file if logging is enabled
    //
    if (_Merger_OpenLogFile(config) != 0) {
        free(_merger_state.buffer);
        _merger_state.buffer = NULL;
        SYS_MutexDestroy(&_merger_state.mutex);
        _merger_state.mutex_initialized = false;
        SYS_MutexDestroy(&_merger_state.operation_mutex);
        _merger_state.operation_mutex_initialized = false;
        return -3;
    }
    //
    // Mark as initialized
    //
    _merger_state.initialized          = true;
    _merger_state.entry_count          = 0;
    _merger_state.total_processed      = 0;
    _merger_state.last_insert_tick     = 0;
    _merger_state.first_buffered_tick  = 0;

    return 0;
}

/*********************************************************************
*
*       LogMerger_Cleanup()
*
*  Function description
*    Cleanup log merger resources and free buffers.
*/
void LogMerger_Cleanup(void) {
    unsigned i;
    //
    // Lock operation and state mutexes during cleanup
    //
    if (_merger_state.operation_mutex_initialized) {
        SYS_MutexLock(&_merger_state.operation_mutex);
    }
    //
    // Lock state mutex during cleanup
    //
    if (_merger_state.mutex_initialized) {
        SYS_MutexLock(&_merger_state.mutex);
    }
    //
    // Free any remaining buffered entries
    //
    if (_merger_state.buffer != NULL) {
        for (i = 0; i < _merger_state.entry_count; i++) {
            if (_merger_state.buffer[i] != NULL) {
                LogEntry_Destroy(_merger_state.buffer[i]);
                _merger_state.buffer[i] = NULL;
            }
        }
        free(_merger_state.buffer);
        _merger_state.buffer = NULL;
    }
    //
    // Close log file if opened
    //
    _Merger_CloseLogFileLocked();
    //
    // Mark as not initialized before unlocking
    //
    _merger_state.initialized = false;
    //
    // Unlock and destroy mutex
    //
    if (_merger_state.mutex_initialized) {
        SYS_MutexUnlock(&_merger_state.mutex);
        SYS_MutexDestroy(&_merger_state.mutex);
        _merger_state.mutex_initialized = false;
    }
    if (_merger_state.operation_mutex_initialized) {
        SYS_MutexUnlock(&_merger_state.operation_mutex);
        SYS_MutexDestroy(&_merger_state.operation_mutex);
        _merger_state.operation_mutex_initialized = false;
    }
    //
    // Reset remaining state
    //
    _merger_state.entry_count          = 0;
    _merger_state.total_processed      = 0;
    _merger_state.last_insert_tick     = 0;
    _merger_state.first_buffered_tick  = 0;
}

/*********************************************************************
*
*       LogMerger_Insert()
*
*  Function description
*    Insert a log entry into the merger for sorting.
*    The merger takes ownership of the entry.
*
*  Parameters
*    entry  Pointer to LogEntry_t to insert
*
*  Return value
*    0   Success
*   -1   Merger not initialized
*   -2   Buffer full
*   -3   Invalid entry
*   -4   Entry is older than already delivered output
*/
int LogMerger_Insert(LogEntry_t *entry) {
    int result;
    //
    // Check initialization
    //
    if (!_merger_state.initialized) {
        LogEntry_Destroy(entry);
        return -1;
    }
    //
    // Validate entry
    //
    if (!LogEntry_IsValid(entry) || !_Merger_EntrySourceIsRequired(entry)) {
        LogEntry_Destroy(entry);
        return -3;
    }
    //
    // Serialize insert against flush and process operations.
    //
    SYS_MutexLock(&_merger_state.operation_mutex);
    //
    // Lock mutex for thread safety
    //
    SYS_MutexLock(&_merger_state.mutex);
    result = _Merger_InsertEntryAndUpdateWatermark(entry);
    if (result == 0) {
        entry = NULL;
    } else if (result == -1) {
        result = -2;
    } else {
        result = -4;
    }
    //
    // Unlock mutex
    //
    SYS_MutexUnlock(&_merger_state.mutex);
    SYS_MutexUnlock(&_merger_state.operation_mutex);

    if (result != 0) {
        LogEntry_Destroy(entry);
    }
    return result;
}

/*********************************************************************
*
*       LogMerger_FlushReady()
*
*  Function description
*    Flush entries that are safe to deliver according to source
*    timestamp watermarks.
*
*  Parameters
*    output     Output callback function
*    user_data  User context for callback
*
*  Return value
*    >= 0  Number of entries flushed
*    -1    Merger not initialized
*    -2    Internal allocation failed or invalid callback
*    -4    Output callback requested stop
*/
int LogMerger_FlushReady(LogMerger_Output_t output, void *user_data) {
    unsigned      flushed;
    unsigned      count;
    LogEntry_t  **temp_buffer;
    bool          stopped;

    if (!_merger_state.initialized) {
        return -1;
    }
    if (output == NULL) {
        return -2;
    }

    SYS_MutexLock(&_merger_state.operation_mutex);
    SYS_MutexLock(&_merger_state.mutex);
    if (_Merger_DetachReadyEntries(&temp_buffer, &count) != 0) {
        SYS_MutexUnlock(&_merger_state.mutex);
        SYS_MutexUnlock(&_merger_state.operation_mutex);
        return -2;
    }
    SYS_MutexUnlock(&_merger_state.mutex);

    if (count == 0) {
        SYS_MutexUnlock(&_merger_state.operation_mutex);
        return 0;
    }

    flushed = _Merger_DeliverDetachedEntries(temp_buffer, count, output, user_data, &stopped);
    free(temp_buffer);

    SYS_MutexLock(&_merger_state.mutex);
    _merger_state.total_processed += flushed;
    SYS_MutexUnlock(&_merger_state.mutex);
    SYS_MutexUnlock(&_merger_state.operation_mutex);

    return stopped ? -4 : (int)flushed;
}

/*********************************************************************
*
*       LogMerger_Flush()
*
*  Function description
*    Flush all buffered entries in sorted order via output callback.
*    Use during shutdown after collectors have stopped.
*
*  Parameters
*    output     Output callback function
*    user_data  User context for callback
*
*  Return value
*    >= 0  Number of entries flushed
*    -1    Merger not initialized
*    -2    Internal allocation failed or invalid callback
*    -4    Output callback requested stop
*/
int LogMerger_Flush(LogMerger_Output_t output, void *user_data) {
    unsigned      flushed;
    unsigned      count;
    LogEntry_t  **temp_buffer;
    bool          stopped;
    //
    // Check initialization
    //
    if (!_merger_state.initialized) {
        return -1;
    }
    if (output == NULL) {
        return -2;
    }
    //
    // Serialize the entire detach and delivery sequence.
    //
    SYS_MutexLock(&_merger_state.operation_mutex);
    //
    // Lock mutex for thread safety
    //
    SYS_MutexLock(&_merger_state.mutex);
    if (_Merger_DetachBufferedEntries(&temp_buffer, &count) != 0) {
        SYS_MutexUnlock(&_merger_state.mutex);
        SYS_MutexUnlock(&_merger_state.operation_mutex);
        return -2;
    }
    //
    // Unlock mutex before calling callbacks.
    //
    SYS_MutexUnlock(&_merger_state.mutex);
    if (count == 0) {
        SYS_MutexUnlock(&_merger_state.operation_mutex);
        return 0;
    }
    //
    // Output all entries in order.
    //
    flushed = _Merger_DeliverDetachedEntries(temp_buffer, count, output, user_data, &stopped);
    //
    // Free temporary buffer.
    //
    free(temp_buffer);
    //
    // Update counters.
    //
    SYS_MutexLock(&_merger_state.mutex);
    _merger_state.total_processed += flushed;
    SYS_MutexUnlock(&_merger_state.mutex);
    SYS_MutexUnlock(&_merger_state.operation_mutex);

    return stopped ? -4 : (int)flushed;
}

/*********************************************************************
*
*       LogMerger_Process()
*
*  Function description
*    Insert entry and automatically flush if threshold reached.
*    This is a convenience function combining Insert and conditional Flush.
*    The merger takes ownership of entry on every return path.
*
*  Parameters
*    entry      Pointer to LogEntry_t to process
*    output     Output callback function
*    user_data  User context for callback
*
*  Return value
*    0   Success
*   -1   Merger not initialized
*   -2   Invalid entry
*   -3   Internal allocation failed or invalid callback
*   -4   Output callback requested stop
*   -5   Entry ordering cannot be preserved
*/
int LogMerger_Process(LogEntry_t *entry, LogMerger_Output_t output, void *user_data) {
    int           result;
    bool          need_flush;
    bool          stopped;
    unsigned      count;
    LogEntry_t  **temp_buffer;
    unsigned      flushed;
    unsigned      now;
    bool          buffer_age_expired;
    bool          insert_idle_expired;
    //
    // Check initialization
    //
    if (!_merger_state.initialized) {
        LogEntry_Destroy(entry);
        return -1;
    }
    if (output == NULL) {
        LogEntry_Destroy(entry);
        return -3;
    }
    //
    // Validate entry
    //
    if (!LogEntry_IsValid(entry) || !_Merger_EntrySourceIsRequired(entry)) {
        LogEntry_Destroy(entry);
        return -2;
    }
    //
    // Serialize process against flush and insert operations.
    //
    SYS_MutexLock(&_merger_state.operation_mutex);
    //
    // Lock mutex for atomic insert
    //
    SYS_MutexLock(&_merger_state.mutex);
    if (_Merger_EntryIsBeforeLastDelivered(entry)) {
        SYS_MutexUnlock(&_merger_state.mutex);
        SYS_MutexUnlock(&_merger_state.operation_mutex);
        LogEntry_Destroy(entry);
        return -5;
    }
    //
    // Flush stale entries before inserting a newer entry.
    //
    now = SYS_GetTickCount();
    buffer_age_expired = _merger_state.first_buffered_tick != 0u &&
                         (unsigned)(now - _merger_state.first_buffered_tick) >=
                         _merger_state.flush_timeout_ms;
    insert_idle_expired = _merger_state.last_insert_tick != 0u &&
                          (unsigned)(now - _merger_state.last_insert_tick) >=
                          _merger_state.flush_timeout_ms;
    if (_merger_state.entry_count > 0 &&
        _merger_state.flush_timeout_ms > 0 &&
        (buffer_age_expired || insert_idle_expired)) {
        if (_Merger_DetachReadyEntriesLimited(&temp_buffer, &count, entry) != 0) {
            SYS_MutexUnlock(&_merger_state.mutex);
            SYS_MutexUnlock(&_merger_state.operation_mutex);
            LogEntry_Destroy(entry);
            return -3;
        }
        if (count > 0) {
            SYS_MutexUnlock(&_merger_state.mutex);

            flushed = _Merger_DeliverDetachedEntries(temp_buffer, count, output, user_data, &stopped);
            free(temp_buffer);

            SYS_MutexLock(&_merger_state.mutex);
            _merger_state.total_processed += flushed;
            if (stopped) {
                SYS_MutexUnlock(&_merger_state.mutex);
                SYS_MutexUnlock(&_merger_state.operation_mutex);
                LogEntry_Destroy(entry);
                return -4;
            }
        }
    }
    //
    // Try insert entry
    //
    result = _Merger_InsertEntryAndUpdateWatermark(entry);
    if (result == -1) {
        int wait_result;

        if (_Merger_DetachReadyEntriesLimited(&temp_buffer, &count, entry) != 0) {
            SYS_MutexUnlock(&_merger_state.mutex);
            SYS_MutexUnlock(&_merger_state.operation_mutex);
            LogEntry_Destroy(entry);
            return -3;
        }
        if (count == 0) {
            wait_result = _Merger_WaitForCapacityReadyEntries(&temp_buffer, &count, entry);
            if (wait_result < 0) {
                SYS_MutexUnlock(&_merger_state.mutex);
                SYS_MutexUnlock(&_merger_state.operation_mutex);
                LogEntry_Destroy(entry);
                return -3;
            }
            if (wait_result == 0 || count == 0u) {
                SYS_MutexUnlock(&_merger_state.mutex);
                SYS_MutexUnlock(&_merger_state.operation_mutex);
                LogEntry_Destroy(entry);
                return -5;
            }
        }
        //
        // Unlock for callbacks
        //
        SYS_MutexUnlock(&_merger_state.mutex);

        flushed = _Merger_DeliverDetachedEntries(temp_buffer, count, output, user_data, &stopped);
        free(temp_buffer);
        //
        // Update counters and try insert again
        //
        SYS_MutexLock(&_merger_state.mutex);
        _merger_state.total_processed += flushed;
        if (stopped) {
            SYS_MutexUnlock(&_merger_state.mutex);
            SYS_MutexUnlock(&_merger_state.operation_mutex);
            LogEntry_Destroy(entry);
            return -4;
        }
        result = _Merger_InsertEntryAndUpdateWatermark(entry);
        if (result != 0) {
            SYS_MutexUnlock(&_merger_state.mutex);
            SYS_MutexUnlock(&_merger_state.operation_mutex);
            LogEntry_Destroy(entry);
            return -5;
        }
    } else if (result != 0) {
        SYS_MutexUnlock(&_merger_state.mutex);
        SYS_MutexUnlock(&_merger_state.operation_mutex);
        LogEntry_Destroy(entry);
        return -5;
    }
    //
    // Check if we should flush based on threshold
    //
    need_flush = (_merger_state.entry_count >= _merger_state.flush_threshold);
    if (need_flush) {
        if (_Merger_DetachReadyEntries(&temp_buffer, &count) != 0) {
            SYS_MutexUnlock(&_merger_state.mutex);
            SYS_MutexUnlock(&_merger_state.operation_mutex);
            return -3;
        }
        if (count > 0) {
            SYS_MutexUnlock(&_merger_state.mutex);

            flushed = _Merger_DeliverDetachedEntries(temp_buffer, count, output, user_data, &stopped);
            free(temp_buffer);

            SYS_MutexLock(&_merger_state.mutex);
            _merger_state.total_processed += flushed;
            if (stopped) {
                SYS_MutexUnlock(&_merger_state.mutex);
                SYS_MutexUnlock(&_merger_state.operation_mutex);
                return -4;
            }
        }
    }
    //
    // Unlock mutex
    //
    SYS_MutexUnlock(&_merger_state.mutex);
    SYS_MutexUnlock(&_merger_state.operation_mutex);

    return 0;
}

/*********************************************************************
*
*       LogMerger_GetBufferedCount()
*
*  Function description
*    Get the number of entries currently buffered.
*
*  Return value
*    Number of buffered entries
*/
unsigned LogMerger_GetBufferedCount(void) {
    unsigned count;

    if (!_merger_state.initialized || !_merger_state.mutex_initialized) {
        return 0;
    }
    SYS_MutexLock(&_merger_state.mutex);
    count = _merger_state.entry_count;
    SYS_MutexUnlock(&_merger_state.mutex);
    return count;
}

/*************************** End of file ****************************/
