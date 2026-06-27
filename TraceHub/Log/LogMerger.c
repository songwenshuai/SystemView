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
File    : LogMerger.c
Purpose : Log merger implementation
---------------------------END-OF-HEADER------------------------------
*/

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "LogMerger.h"
#include "Log.h"
#include "SYS.h"

/*********************************************************************
*
*       Defines, fixed
*
**********************************************************************
*/

#define LOG_MERGER_CAPACITY_WAIT_SLICE_MS  10u

/*********************************************************************
*
*       Types
*
**********************************************************************
*/

/*********************************************************************
*
*       LogMerger_State_t
*
*  Description
*    Runtime state of the log merger.
*/
typedef struct {
    LogMerger_Config_t   config;
    bool                 initialized;
    LogEntry_t         **buffer;
    unsigned             buffer_size;
    unsigned             entry_count;
    unsigned             total_processed;
    unsigned             last_insert_tick;
    unsigned             first_buffered_tick;
    uint64_t             source_watermark[LOG_SOURCE_MAX];
    unsigned             source_last_tick[LOG_SOURCE_MAX];
    bool                 source_seen[LOG_SOURCE_MAX];
    uint64_t             last_delivered_timestamp;
    LogSource_t          last_delivered_source;
    uint64_t             last_delivered_sequence;
    bool                 last_delivered_valid;
    FILE                *log_file;
    SYS_Mutex            operation_mutex;
    bool                 operation_mutex_initialized;
    SYS_Mutex            mutex;
    bool                 mutex_initialized;
    bool                 log_file_error;
} LogMerger_State_t;

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

static LogMerger_State_t _merger_state;

/*********************************************************************
*
*       Static functions
*
**********************************************************************
*/

/*********************************************************************
*
*       _ReportLogFileErrorLocked()
*
*  Function description
*    Record a merger log file persistence error. Caller holds mutex.
*/
static void _ReportLogFileErrorLocked(const char *operation, int error_code) {
    if (operation == NULL) {
        operation = "operation";
    }
    if (!_merger_state.log_file_error) {
        if (error_code != 0) {
            fprintf(stderr,
                    "[LogMerger] log file %s failed: %s\n",
                    operation, strerror(error_code));
        } else {
            fprintf(stderr,
                    "[LogMerger] log file %s failed\n",
                    operation);
        }
    }
    _merger_state.log_file_error = true;
}

/*********************************************************************
*
*       _InsertSorted()
*
*  Function description
*    Insert entry into sorted position in buffer.
*    Uses insertion sort for small buffers.
*
*  Parameters
*    entry  Entry to insert
*
*  Return value
*    0   Success
*   -1   Buffer full
*/
static int _InsertSorted(LogEntry_t *entry) {
    unsigned i;
    unsigned insert_pos;
    //
    // Check if buffer is full
    //
    if (_merger_state.entry_count >= _merger_state.buffer_size) {
        return -1;
    }
    //
    // Find insertion position (binary search would be faster for large buffers)
    //
    insert_pos = _merger_state.entry_count;

    for (i = 0; i < _merger_state.entry_count; i++) {
        if (LogEntry_Compare(entry, _merger_state.buffer[i]) < 0) {
            insert_pos = i;
            break;
        }
    }
    //
    // Shift entries to make room
    //
    for (i = _merger_state.entry_count; i > insert_pos; i--) {
        _merger_state.buffer[i] = _merger_state.buffer[i - 1];
    }
    //
    // Insert entry
    //
    _merger_state.buffer[insert_pos] = entry;
    _merger_state.entry_count++;

    return 0;
}

/*********************************************************************
*
*       _RemoveBufferedEntry()
*
*  Function description
*    Remove one buffered entry pointer without destroying it.
*    The caller must hold the merger mutex.
*/
static void _RemoveBufferedEntry(LogEntry_t *entry) {
    unsigned i;
    unsigned remaining_count;

    if (entry == NULL) {
        return;
    }

    for (i = 0; i < _merger_state.entry_count; i++) {
        if (_merger_state.buffer[i] != entry) {
            continue;
        }

        remaining_count = _merger_state.entry_count - i - 1u;
        if (remaining_count > 0u) {
            memmove(&_merger_state.buffer[i],
                    &_merger_state.buffer[i + 1u],
                    sizeof(LogEntry_t *) * remaining_count);
        }
        _merger_state.entry_count--;
        _merger_state.buffer[_merger_state.entry_count] = NULL;
        if (_merger_state.entry_count == 0u) {
            _merger_state.first_buffered_tick = 0u;
        }
        return;
    }
}

/*********************************************************************
*
*       _DetachBufferedEntries()
*
*  Function description
*    Detach buffered entries into a temporary array.
*    The caller must hold the merger mutex.
*
*  Parameters
*    entries  Receives detached entry pointer array
*    count    Receives detached entry count
*
*  Return value
*    0   Success
*   -1   Allocation failed
*/
static int _DetachBufferedEntries(LogEntry_t ***entries, unsigned *count) {
    LogEntry_t **temp_buffer;

    *entries = NULL;
    *count   = _merger_state.entry_count;

    if (*count == 0) {
        return 0;
    }

    temp_buffer = (LogEntry_t **)malloc(sizeof(LogEntry_t *) * (*count));
    if (temp_buffer == NULL) {
        return -1;
    }

    memcpy(temp_buffer, _merger_state.buffer, sizeof(LogEntry_t *) * (*count));
    memset(_merger_state.buffer, 0, sizeof(LogEntry_t *) * (*count));
    _merger_state.entry_count = 0;
    _merger_state.first_buffered_tick = 0u;

    *entries = temp_buffer;
    return 0;
}

/*********************************************************************
*
*       _SourceIsRequired()
*
*  Function description
*    Check whether a source participates in merge ordering.
*/
static bool _SourceIsRequired(LogSource_t source) {
    if (source >= LOG_SOURCE_MAX) {
        return false;
    }

    return _merger_state.config.required_source[source];
}

/*********************************************************************
*
*       _EntrySourceIsRequired()
*
*  Function description
*    Check whether an entry source participates in merge ordering.
*/
static bool _EntrySourceIsRequired(const LogEntry_t *entry) {
    if (!LogEntry_IsValid(entry)) {
        return false;
    }

    return _SourceIsRequired(LogEntry_GetSource(entry));
}

/*********************************************************************
*
*       _GetReadyWatermark()
*
*  Function description
*    Get the timestamp watermark for entries that can be delivered.
*    Required sources that have not appeared yet block delivery until the
*    oldest buffered entry has waited past flush_timeout_ms. Required sources
*    that have appeared but then go idle are ignored after the same timeout.
*
*  Return value
*    true   Watermark available
*    false  No buffered entry is available
*/
static bool _GetReadyWatermark(uint64_t *watermark) {
    uint64_t result;
    unsigned now;
    unsigned timeout_ms;
    unsigned i;
    bool     have_active_source;
    bool     buffer_wait_expired;

    if (watermark == NULL) {
        return false;
    }
    if (_merger_state.entry_count == 0) {
        return false;
    }

    result = UINT64_MAX;
    now = SYS_GetTickCount();
    timeout_ms = _merger_state.config.flush_timeout_ms;
    have_active_source = false;
    buffer_wait_expired = timeout_ms > 0u &&
                          _merger_state.first_buffered_tick != 0u &&
                          (unsigned)(now - _merger_state.first_buffered_tick) >= timeout_ms;

    for (i = 0; i < LOG_SOURCE_MAX; i++) {
        if (!_merger_state.config.required_source[i]) {
            continue;
        }
        if (!_merger_state.source_seen[i]) {
            if (!buffer_wait_expired) {
                return false;
            }
            continue;
        }
        if (timeout_ms > 0 &&
            (unsigned)(now - _merger_state.source_last_tick[i]) >= timeout_ms) {
            continue;
        }
        have_active_source = true;
        if (_merger_state.source_watermark[i] < result) {
            result = _merger_state.source_watermark[i];
        }
    }

    if (!have_active_source) {
        result = LogEntry_GetTimestamp(_merger_state.buffer[_merger_state.entry_count - 1]);
    }

    *watermark = result;
    return true;
}

/*********************************************************************
*
*       _DetachReadyEntriesLimited()
*
*  Function description
*    Detach buffered entries whose timestamps are not newer than the
*    current source watermark. The caller must hold the merger mutex.
*
*  Parameters
*    entries             Receives detached entry pointer array
*    count               Receives detached entry count
*    limit_entry         Pending entry that bounds delivered ordering
*
*  Return value
*    0   Success
*   -1   Allocation failed
*/
static int _DetachReadyEntriesLimited(LogEntry_t ***entries, unsigned *count,
                                      const LogEntry_t *limit_entry) {
    LogEntry_t **temp_buffer;
    uint64_t     watermark;
    unsigned     ready_count;
    unsigned     remaining_count;

    *entries = NULL;
    *count   = 0;

    if (_merger_state.entry_count == 0) {
        return 0;
    }
    if (!_GetReadyWatermark(&watermark)) {
        return 0;
    }

    ready_count = 0;
    while (ready_count < _merger_state.entry_count &&
           LogEntry_GetTimestamp(_merger_state.buffer[ready_count]) <= watermark) {
        if (limit_entry != NULL &&
            LogEntry_Compare(_merger_state.buffer[ready_count], limit_entry) > 0) {
            break;
        }
        ready_count++;
    }
    if (ready_count == 0) {
        return 0;
    }

    temp_buffer = (LogEntry_t **)malloc(sizeof(LogEntry_t *) * ready_count);
    if (temp_buffer == NULL) {
        return -1;
    }

    memcpy(temp_buffer, _merger_state.buffer, sizeof(LogEntry_t *) * ready_count);

    remaining_count = _merger_state.entry_count - ready_count;
    if (remaining_count > 0) {
        memmove(_merger_state.buffer,
                &_merger_state.buffer[ready_count],
                sizeof(LogEntry_t *) * remaining_count);
    }
    memset(&_merger_state.buffer[remaining_count],
           0,
           sizeof(LogEntry_t *) * ready_count);
    _merger_state.entry_count = remaining_count;
    if (_merger_state.entry_count == 0u) {
        _merger_state.first_buffered_tick = 0u;
    } else {
        _merger_state.first_buffered_tick = SYS_GetTickCount();
    }

    *entries = temp_buffer;
    *count   = ready_count;
    return 0;
}

/*********************************************************************
*
*       _DetachReadyEntries()
*
*  Function description
*    Detach watermark-ready buffered entries.
*    The caller must hold the merger mutex.
*/
static int _DetachReadyEntries(LogEntry_t ***entries, unsigned *count) {
    return _DetachReadyEntriesLimited(entries, count, NULL);
}

/*********************************************************************
*
*       _WaitForCapacityReadyEntries()
*
*  Function description
*    Wait until the configured timeout makes at least one buffered entry
*    safe to detach. The caller holds operation_mutex and mutex. The mutex
*    is released while sleeping and held again on return.
*
*  Return value
*     1  Ready entries detached
*     0  No ready entries became available
*    -1  Allocation failed
*/
static int _WaitForCapacityReadyEntries(LogEntry_t ***entries, unsigned *count,
                                        const LogEntry_t *limit_entry) {
    unsigned wait_start_tick;
    unsigned timeout_ms;

    *entries = NULL;
    *count   = 0u;

    timeout_ms = _merger_state.config.flush_timeout_ms;
    if (timeout_ms == 0u) {
        return 0;
    }

    wait_start_tick = SYS_GetTickCount();
    for (;;) {
        unsigned now;
        unsigned elapsed;
        unsigned wait_ms;

        if (_DetachReadyEntriesLimited(entries, count, limit_entry) != 0) {
            return -1;
        }
        if (*count > 0u) {
            return 1;
        }

        now = SYS_GetTickCount();
        elapsed = (unsigned)(now - wait_start_tick);
        if (elapsed >= timeout_ms) {
            return 0;
        }

        wait_ms = timeout_ms - elapsed;
        if (wait_ms > LOG_MERGER_CAPACITY_WAIT_SLICE_MS) {
            wait_ms = LOG_MERGER_CAPACITY_WAIT_SLICE_MS;
        }
        if (wait_ms == 0u) {
            wait_ms = 1u;
        }

        SYS_MutexUnlock(&_merger_state.mutex);
        SYS_Sleep(wait_ms);
        SYS_MutexLock(&_merger_state.mutex);
    }
}

/*********************************************************************
*
*       _EntryIsBeforeLastDelivered()
*
*  Function description
*    Check whether an entry would violate delivered output ordering.
*/
static bool _EntryIsBeforeLastDelivered(const LogEntry_t *entry) {
    uint64_t    timestamp;
    uint64_t    sequence;
    LogSource_t source;

    if (!_merger_state.last_delivered_valid) {
        return false;
    }

    timestamp = LogEntry_GetTimestamp(entry);
    source    = LogEntry_GetSource(entry);
    sequence  = LogEntry_GetSequence(entry);

    if (timestamp < _merger_state.last_delivered_timestamp) {
        return true;
    }
    if (timestamp > _merger_state.last_delivered_timestamp) {
        return false;
    }

    if (source < _merger_state.last_delivered_source) {
        return true;
    }
    if (source > _merger_state.last_delivered_source) {
        return false;
    }

    return sequence < _merger_state.last_delivered_sequence;
}

/*********************************************************************
*
*       _UpdateSourceWatermark()
*
*  Function description
*    Record the newest timestamp seen for an entry source.
*
*  Return value
*    0   Success
*   -1   Entry would violate already delivered ordering
*/
static int _UpdateSourceWatermark(const LogEntry_t *entry) {
    uint64_t    timestamp;
    LogSource_t source;
    unsigned    now;

    if (!_EntrySourceIsRequired(entry) ||
        _EntryIsBeforeLastDelivered(entry)) {
        return -1;
    }

    timestamp = LogEntry_GetTimestamp(entry);
    source    = LogEntry_GetSource(entry);
    now       = SYS_GetTickCount();

    if (!_merger_state.source_seen[source] ||
        timestamp > _merger_state.source_watermark[source]) {
        _merger_state.source_watermark[source] = timestamp;
    }
    _merger_state.source_last_tick[source] = now;
    _merger_state.source_seen[source] = true;

    return 0;
}

/*********************************************************************
*
*       _InsertEntryAndUpdateWatermark()
*
*  Function description
*    Insert an entry and update the source watermark as one ordered state
*    transition. The caller must hold the merger mutex.
*
*  Return value
*     0  Success
*    -1  Buffer full
*    -2  Entry would violate already delivered ordering
*/
static int _InsertEntryAndUpdateWatermark(LogEntry_t *entry) {
    unsigned now;
    bool     was_empty;

    if (_EntryIsBeforeLastDelivered(entry)) {
        return -2;
    }

    now       = SYS_GetTickCount();
    was_empty = (_merger_state.entry_count == 0u);
    if (_InsertSorted(entry) != 0) {
        return -1;
    }
    if (_UpdateSourceWatermark(entry) != 0) {
        _RemoveBufferedEntry(entry);
        return -2;
    }

    if (was_empty || _merger_state.first_buffered_tick == 0u) {
        _merger_state.first_buffered_tick = now;
    }
    _merger_state.last_insert_tick = now;
    return 0;
}

/*********************************************************************
*
*       _DeliverDetachedEntries()
*
*  Function description
*    Deliver detached entries in order.
*
*  Parameters
*    entries    Detached entry pointer array
*    count      Number of entries in array
*    output     Output callback
*    user_data  User context for callback
*    stopped    Receives callback stop request state
*
*  Return value
*    Number of entries successfully delivered.
*/
static unsigned _DeliverDetachedEntries(LogEntry_t **entries, unsigned count,
                                        LogMerger_Output_t output, void *user_data,
                                        bool *stopped) {
    unsigned i;
    unsigned flushed;

    *stopped = false;
    flushed  = 0;

    for (i = 0; i < count; i++) {
        LogEntry_t *entry;
        int         result;
        uint64_t    timestamp;
        uint64_t    sequence;
        LogSource_t source;

        if (entries[i] == NULL) {
            continue;
        }

        entry      = entries[i];
        entries[i] = NULL;
        timestamp  = LogEntry_GetTimestamp(entry);
        source     = LogEntry_GetSource(entry);
        sequence   = LogEntry_GetSequence(entry);
        result     = output(entry, user_data);
        if (result != 0) {
            unsigned j;

            *stopped = true;
            for (j = i + 1; j < count; j++) {
                if (entries[j] != NULL) {
                    LogEntry_Destroy(entries[j]);
                    entries[j] = NULL;
                }
            }
            break;
        }

        flushed++;
        _merger_state.last_delivered_timestamp = timestamp;
        _merger_state.last_delivered_source    = source;
        _merger_state.last_delivered_sequence  = sequence;
        _merger_state.last_delivered_valid     = true;
    }

    return flushed;
}

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
    if (config->flush_threshold == 0 ||
        config->flush_threshold > config->buffer_size) {
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
    if (config->log_enabled && config->log_prefix != NULL) {
        _merger_state.log_file = LOG_CreateTimestampedFile(config->log_prefix);
        if (_merger_state.log_file == NULL) {
            free(_merger_state.buffer);
            _merger_state.buffer = NULL;
            SYS_MutexDestroy(&_merger_state.mutex);
            _merger_state.mutex_initialized = false;
            SYS_MutexDestroy(&_merger_state.operation_mutex);
            _merger_state.operation_mutex_initialized = false;
            return -3;
        }
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
    if (_merger_state.log_file != NULL) {
        FILE *log_file;

        log_file = _merger_state.log_file;
        _merger_state.log_file = NULL;
        errno = 0;
        if (fclose(log_file) != 0) {
            _ReportLogFileErrorLocked("close", errno);
        }
    }
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
    if (!LogEntry_IsValid(entry) || !_EntrySourceIsRequired(entry)) {
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
    result = _InsertEntryAndUpdateWatermark(entry);
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
    if (_DetachReadyEntries(&temp_buffer, &count) != 0) {
        SYS_MutexUnlock(&_merger_state.mutex);
        SYS_MutexUnlock(&_merger_state.operation_mutex);
        return -2;
    }
    SYS_MutexUnlock(&_merger_state.mutex);

    if (count == 0) {
        SYS_MutexUnlock(&_merger_state.operation_mutex);
        return 0;
    }

    flushed = _DeliverDetachedEntries(temp_buffer, count, output, user_data, &stopped);
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
    if (_DetachBufferedEntries(&temp_buffer, &count) != 0) {
        SYS_MutexUnlock(&_merger_state.mutex);
        SYS_MutexUnlock(&_merger_state.operation_mutex);
        return -2;
    }
    //
    // Unlock mutex before calling callbacks (callbacks may take time)
    //
    SYS_MutexUnlock(&_merger_state.mutex);
    if (count == 0) {
        SYS_MutexUnlock(&_merger_state.operation_mutex);
        return 0;
    }
    //
    // Output all entries in order (without holding lock)
    //
    flushed = _DeliverDetachedEntries(temp_buffer, count, output, user_data, &stopped);
    //
    // Free temporary buffer
    //
    free(temp_buffer);
    //
    // Update counters (need lock)
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
    if (!LogEntry_IsValid(entry) || !_EntrySourceIsRequired(entry)) {
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
    if (_EntryIsBeforeLastDelivered(entry)) {
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
                         _merger_state.config.flush_timeout_ms;
    insert_idle_expired = _merger_state.last_insert_tick != 0u &&
                          (unsigned)(now - _merger_state.last_insert_tick) >=
                          _merger_state.config.flush_timeout_ms;
    if (_merger_state.entry_count > 0 &&
        _merger_state.config.flush_timeout_ms > 0 &&
        (buffer_age_expired || insert_idle_expired)) {
        if (_DetachReadyEntriesLimited(&temp_buffer, &count, entry) != 0) {
            SYS_MutexUnlock(&_merger_state.mutex);
            SYS_MutexUnlock(&_merger_state.operation_mutex);
            LogEntry_Destroy(entry);
            return -3;
        }
        if (count > 0) {
            SYS_MutexUnlock(&_merger_state.mutex);

            flushed = _DeliverDetachedEntries(temp_buffer, count, output, user_data, &stopped);
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
    result = _InsertEntryAndUpdateWatermark(entry);
    if (result == -1) {
        int wait_result;

        if (_DetachReadyEntriesLimited(&temp_buffer, &count, entry) != 0) {
            SYS_MutexUnlock(&_merger_state.mutex);
            SYS_MutexUnlock(&_merger_state.operation_mutex);
            LogEntry_Destroy(entry);
            return -3;
        }
        if (count == 0) {
            wait_result = _WaitForCapacityReadyEntries(&temp_buffer, &count, entry);
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

        flushed = _DeliverDetachedEntries(temp_buffer, count, output, user_data, &stopped);
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
        result = _InsertEntryAndUpdateWatermark(entry);
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
    need_flush = (_merger_state.entry_count >= _merger_state.config.flush_threshold);
    if (need_flush) {
        if (_DetachReadyEntries(&temp_buffer, &count) != 0) {
            SYS_MutexUnlock(&_merger_state.mutex);
            SYS_MutexUnlock(&_merger_state.operation_mutex);
            return -3;
        }
        if (count > 0) {
            SYS_MutexUnlock(&_merger_state.mutex);

            flushed = _DeliverDetachedEntries(temp_buffer, count, output, user_data, &stopped);
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

/*********************************************************************
*
*       LogMerger_WriteEntry()
*
*  Function description
*    Persist a merged entry to the merger-owned log file when logging is
*    enabled.
*
*  Return value
*    0   Success or logging disabled.
*   -1   Log file write failed.
*/
int LogMerger_WriteEntry(const LogEntry_t *entry) {
    const char *source_name;
    int         result;
    int         error_code;

    if (entry == NULL) {
        return -1;
    }
    if (!_merger_state.initialized || !_merger_state.mutex_initialized) {
        return -1;
    }

    SYS_MutexLock(&_merger_state.mutex);
    if (_merger_state.log_file == NULL) {
        SYS_MutexUnlock(&_merger_state.mutex);
        return 0;
    }

    source_name = (LogEntry_GetSource(entry) == LOG_SOURCE_LINUX) ? "LINUX" : "RTOS";
    errno = 0;
    result = LOG_SwimLaneLogToFile(_merger_state.log_file,
                                   LogEntry_GetTimestamp(entry),
                                   source_name,
                                   LogEntry_GetContent(entry));
    error_code = errno;
    if (result != 0) {
        _ReportLogFileErrorLocked("write", error_code);
        SYS_MutexUnlock(&_merger_state.mutex);
        return -1;
    }

    SYS_MutexUnlock(&_merger_state.mutex);
    return 0;
}

/*********************************************************************
*
*       LogMerger_HasFileError()
*
*  Function description
*    Check whether the merger log file encountered a persistence error.
*
*  Return value
*    true   Persistence error occurred.
*    false  No persistence error recorded.
*/
bool LogMerger_HasFileError(void) {
    bool file_error;

    if (!_merger_state.mutex_initialized) {
        return _merger_state.log_file_error;
    }

    SYS_MutexLock(&_merger_state.mutex);
    file_error = _merger_state.log_file_error;
    SYS_MutexUnlock(&_merger_state.mutex);
    return file_error;
}

/*************************** End of file ****************************/
