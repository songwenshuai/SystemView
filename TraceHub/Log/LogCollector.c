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
File    : LogCollector.c
Purpose : Log collector implementation for dual RTT channels
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

#include "LogCollector.h"
#include "CoreLogRecorder.h"
#include "RTTBridge.h"
#include "SYS.h"
#include "LogLineParser.h"

/*********************************************************************
*
*       Defines
*
**********************************************************************
*/

#define LOG_COLLECT_RESULT_NO_DATA      (-1)
#define LOG_COLLECT_RESULT_INVALID_RTT  (-2)
#define LOG_COLLECT_RESULT_STOP         (-3)
#define LOG_COLLECT_RESULT_PENDING_OVERFLOW (-4)

#define LOG_COLLECTOR_PENDING_FLAG_CONTINUATION  (1u << 0)
#define LOG_COLLECTOR_PENDING_FLAG_CONTINUES     (1u << 1)
#define LOG_COLLECTOR_PENDING_RECORD_OVERHEAD    3u

/*********************************************************************
*
*       Types
*
**********************************************************************
*/

/*********************************************************************
*
*       LogCollector_State_t
*
*  Description
*    Runtime state of the log collector.
*/
typedef struct {
    LogCollector_Config_t   config;
    bool                    initialized;
    bool                    running;
    bool                    lock_initialized;
    bool                    emit_lock_initialized;
    LogCollector_Callback_t callback;
    void                   *user_data;
    SYS_Thread              linux_thread;
    SYS_Thread              rtos_thread;
    SYS_Mutex               lock;
    SYS_Mutex               emit_lock;
    bool                    linux_thread_started;
    bool                    rtos_thread_started;
    bool                    linux_consumer_registered;
    bool                    rtos_consumer_registered;
    char                    linux_buffer[LOG_COLLECTOR_MAX_LINE_LEN];
    char                    rtos_buffer[LOG_COLLECTOR_MAX_LINE_LEN];
    size_t                  linux_buffer_len;
    size_t                  rtos_buffer_len;
    uint64_t                linux_last_timestamp;
    uint64_t                rtos_last_timestamp;
    bool                    linux_last_timestamp_valid;
    bool                    rtos_last_timestamp_valid;
    uint64_t                global_last_timestamp;
    bool                    global_last_timestamp_valid;
    char                   *linux_pending_untimed;
    char                   *rtos_pending_untimed;
    size_t                  pending_untimed_size;
    size_t                  linux_pending_untimed_len;
    size_t                  rtos_pending_untimed_len;
    bool                    linux_fragmenting_line;
    bool                    rtos_fragmenting_line;
    uint64_t                linux_sequence;
    uint64_t                rtos_sequence;
    bool                    fatal_error;
    int                     fatal_result;
} LogCollector_State_t;

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

static LogCollector_State_t _collector_state;

/*********************************************************************
*
*       _Collector_FreePendingBuffers()
*
*  Function description
*    Release pending untimestamped buffers owned by the collector.
*/
static void _Collector_FreePendingBuffers(void) {
    free(_collector_state.linux_pending_untimed);
    free(_collector_state.rtos_pending_untimed);
    _collector_state.linux_pending_untimed = NULL;
    _collector_state.rtos_pending_untimed = NULL;
    _collector_state.pending_untimed_size = 0u;
    _collector_state.linux_pending_untimed_len = 0u;
    _collector_state.rtos_pending_untimed_len = 0u;
}

/*********************************************************************
*
*       _Collector_AllocatePendingBuffers()
*
*  Function description
*    Allocate per-source pending untimestamped buffers.
*/
static int _Collector_AllocatePendingBuffers(size_t size) {
    if (size < LOG_COLLECTOR_MIN_PENDING_UNTIMED_SIZE) {
        return -1;
    }

    _collector_state.linux_pending_untimed = (char *)malloc(size);
    if (_collector_state.linux_pending_untimed == NULL) {
        return -1;
    }

    _collector_state.rtos_pending_untimed = (char *)malloc(size);
    if (_collector_state.rtos_pending_untimed == NULL) {
        _Collector_FreePendingBuffers();
        return -1;
    }

    _collector_state.pending_untimed_size = size;
    _collector_state.linux_pending_untimed[0] = '\0';
    _collector_state.rtos_pending_untimed[0] = '\0';
    return 0;
}

/*********************************************************************
*
*       _Collector_IsRunning()
*
*  Function description
*    Read the collector running state through the module lock.
*/
static bool _Collector_IsRunning(void) {
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
*       _Collector_SetRunning()
*
*  Function description
*    Update the collector running state through the module lock.
*/
static void _Collector_SetRunning(bool running) {
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
*       _Collector_LockEmission()
*
*  Function description
*    Serialize timestamp assignment and entry delivery across source threads.
*/
static void _Collector_LockEmission(void) {
    if (_collector_state.emit_lock_initialized) {
        SYS_MutexLock(&_collector_state.emit_lock);
    }
}

/*********************************************************************
*
*       _Collector_UnlockEmission()
*
*  Function description
*    Release the collector emission serialization lock.
*/
static void _Collector_UnlockEmission(void) {
    if (_collector_state.emit_lock_initialized) {
        SYS_MutexUnlock(&_collector_state.emit_lock);
    }
}

/*********************************************************************
*
*       _Collector_RecordObservedTimestamp()
*
*  Function description
*    Record the greatest explicit or inferred timestamp observed by the
*    collector. The value is used to place untimestamped output after all
*    known timed output.
*/
static void _Collector_RecordObservedTimestamp(uint64_t timestamp) {
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
*       _Collector_ReserveUntimedTimestamp()
*
*  Function description
*    Reserve a deterministic timestamp for a line that has no explicit
*    timestamp. The returned value is never older than the greatest timestamp
*    already observed by any source.
*/
static uint64_t _Collector_ReserveUntimedTimestamp(uint64_t requested_timestamp) {
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
    } else {
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
    }

    return requested_timestamp;
}

/*********************************************************************
*
*       _Collector_HasFatalError()
*
*  Function description
*    Read the collector fatal state through the module lock.
*/
static bool _Collector_HasFatalError(void) {
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
*       _Collector_GetFatalResult()
*
*  Function description
*    Read the fatal result through the module lock.
*/
static int _Collector_GetFatalResult(void) {
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
*       _Collector_SetFatalErrorResult()
*
*  Function description
*    Mark the collector as failed with a stable terminal result.
*/
static void _Collector_SetFatalErrorResult(int result) {
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
*       _Collector_SetFatalError()
*
*  Function description
*    Mark the collector as failed and stop background collection.
*/
static void _Collector_SetFatalError(void) {
    _Collector_SetFatalErrorResult(LOG_COLLECT_RESULT_INVALID_RTT);
}

/*********************************************************************
*
*       _Collector_EnterFatalStateWithResult()
*
*  Function description
*    Mark collector as failed and report whether this is the first fatal
*    transition.
*/
static bool _Collector_EnterFatalStateWithResult(int result) {
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
*       _Collector_EnterFatalState()
*
*  Function description
*    Mark collector as failed with the default fatal result.
*/
static bool _Collector_EnterFatalState(void) {
    return _Collector_EnterFatalStateWithResult(LOG_COLLECT_RESULT_INVALID_RTT);
}

/*********************************************************************
*
*       _Collector_ReportRTTError()
*
*  Function description
*    Report an unrecoverable collector RTT or recorder error.
*/
static void _Collector_ReportRTTError(LogSource_t source, const char *operation) {
    bool        should_report;
    const char *source_name;

    if (operation == NULL) {
        operation = "access";
    }

    should_report = _Collector_EnterFatalState();
    if (should_report) {
        source_name = (source == LOG_SOURCE_LINUX) ? "Linux" : "RTOS";
        fprintf(stderr,
                "[LogCollector] %s: fatal %s failure\n",
                source_name,
                operation);
    }
}

/*********************************************************************
*
*       _Collector_ReportCallbackStop()
*
*  Function description
*    Report an unrecoverable callback stop request from background
*    collection.
*/
static void _Collector_ReportCallbackStop(LogSource_t source) {
    const char *source_name;

    if (_Collector_EnterFatalStateWithResult(LOG_COLLECT_RESULT_STOP)) {
        source_name = (source == LOG_SOURCE_LINUX) ? "Linux" : "RTOS";
        fprintf(stderr,
                "[LogCollector] %s: output callback requested stop\n",
                source_name);
    }
}

/*********************************************************************
*
*       _Collector_ReportFlushError()
*
*  Function description
*    Report a pending-line flush failure during shutdown.
*/
static void _Collector_ReportFlushError(int result) {
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

    if (_Collector_EnterFatalStateWithResult(result)) {
        fprintf(stderr, "[LogCollector] fatal shutdown flush failure: %s\n", reason);
    }
}

/*********************************************************************
*
*       _Collector_EnsureConsumersRegistered()
*
*  Function description
*    Ensure LogCollector owns the consumer registrations for both core
*    channels.
*/
static int _Collector_EnsureConsumersRegistered(void) {
    int  result;
    bool linux_registered_here;

    linux_registered_here = false;
    if (!_collector_state.linux_consumer_registered) {
        result = CoreLogRecorder_RegisterConsumer(_collector_state.config.linux_channel);
        if (result != 0) {
            return -1;
        }
        _collector_state.linux_consumer_registered = true;
        linux_registered_here = true;
    }

    if (!_collector_state.rtos_consumer_registered) {
        result = CoreLogRecorder_RegisterConsumer(_collector_state.config.rtos_channel);
        if (result != 0) {
            if (linux_registered_here) {
                CoreLogRecorder_UnregisterConsumer(_collector_state.config.linux_channel);
                _collector_state.linux_consumer_registered = false;
            }
            return -1;
        }
        _collector_state.rtos_consumer_registered = true;
    }

    return 0;
}

/*********************************************************************
*
*       _Collector_UnregisterConsumers()
*
*  Function description
*    Release any core recorder consumer registrations owned by LogCollector.
*/
static void _Collector_UnregisterConsumers(void) {
    if (_collector_state.rtos_consumer_registered) {
        CoreLogRecorder_UnregisterConsumer(_collector_state.config.rtos_channel);
        _collector_state.rtos_consumer_registered = false;
    }
    if (_collector_state.linux_consumer_registered) {
        CoreLogRecorder_UnregisterConsumer(_collector_state.config.linux_channel);
        _collector_state.linux_consumer_registered = false;
    }
}

/*********************************************************************
*
*       _Collector_StopCollectionWithResult()
*
*  Function description
*    Release collector consumers and return a terminal collection result.
*/
static int _Collector_StopCollectionWithResult(int result) {
    if (result == LOG_COLLECT_RESULT_INVALID_RTT ||
        result == LOG_COLLECT_RESULT_PENDING_OVERFLOW) {
        _Collector_SetFatalErrorResult(result);
    }
    _Collector_UnregisterConsumers();
    return result;
}

/*********************************************************************
*
*       _Collector_RecordPendingUntimedOverflow()
*
*  Function description
*    Report that leading untimestamped lines exceeded the pending buffer.
*/
static void _Collector_RecordPendingUntimedOverflow(LogSource_t source, size_t limit) {
    const char *source_name;

    source_name = (source == LOG_SOURCE_LINUX) ? "Linux" : "RTOS";
    fprintf(stderr,
            "[LogCollector] %s: untimestamped startup log exceeds %zu pending bytes; stopping to avoid reordering\n",
            source_name, limit);
    _Collector_SetFatalErrorResult(LOG_COLLECT_RESULT_PENDING_OVERFLOW);
}

/*********************************************************************
*
*       _DeliverLogEntry()
*
*  Function description
*    Create and deliver LogEntry fragments for one content range.
*
*  Return value
*    >0  Entries delivered
*   -1   Entry allocation failed
*   LOG_COLLECT_RESULT_STOP  Callback requested stop
*/
static int _DeliverLogEntry(uint64_t timestamp, LogSource_t source,
                            const char *content, size_t content_len,
                            uint64_t *sequence,
                            bool fragment_continuation,
                            bool fragment_continues,
                            LogCollector_Callback_t callback, void *user_data) {
    size_t offset;
    int    delivered;

    if (content == NULL || content_len == 0u || sequence == NULL || callback == NULL) {
        return -1;
    }

    delivered = 0;
    offset = 0u;
    while (offset < content_len) {
        LogEntry_t *entry;
        size_t      remaining;
        size_t      fragment_len;
        bool        current_continuation;
        bool        current_continues;
        int         result;

        remaining = content_len - offset;
        fragment_len = remaining;
        if (fragment_len > LOG_ENTRY_MAX_CONTENT_LEN) {
            fragment_len = LogLineParser_AdjustUtf8Boundary(content + offset,
                                                            LOG_ENTRY_MAX_CONTENT_LEN);
            if (fragment_len == 0u) {
                fragment_len = LOG_ENTRY_MAX_CONTENT_LEN;
            }
        }

        current_continuation = fragment_continuation || (offset > 0u);
        current_continues = fragment_continues || (fragment_len < remaining);
        entry = LogEntry_CreateEx(timestamp,
                                  source,
                                  *sequence,
                                  current_continuation,
                                  current_continues,
                                  content + offset,
                                  fragment_len);
        if (entry == NULL) {
            return -1;
        }
        (*sequence)++;

        result = callback(entry, user_data);
        if (result != 0) {
            return LOG_COLLECT_RESULT_STOP;
        }

        delivered++;
        offset += fragment_len;
    }

    return delivered;
}

/*********************************************************************
*
*       _AppendPendingUntimedLine()
*
*  Function description
*    Stage an untimestamped line until the source timeline is known.
*/
static int _AppendPendingUntimedLine(LogSource_t source,
                                     char *pending_buffer,
                                     size_t pending_buffer_size,
                                     size_t *pending_len,
                                     const char *content,
                                     size_t content_len,
                                     bool fragment_continuation,
                                     bool fragment_continues) {
    unsigned char flags;

    if (pending_buffer == NULL || pending_len == NULL ||
        content == NULL || content_len == 0u ||
        pending_buffer_size < LOG_COLLECTOR_PENDING_RECORD_OVERHEAD) {
        return -1;
    }

    if (*pending_len > pending_buffer_size - LOG_COLLECTOR_PENDING_RECORD_OVERHEAD ||
        content_len > pending_buffer_size - *pending_len - LOG_COLLECTOR_PENDING_RECORD_OVERHEAD) {
        _Collector_RecordPendingUntimedOverflow(source, pending_buffer_size - 1u);
        return LOG_COLLECT_RESULT_PENDING_OVERFLOW;
    }
    flags = 0u;
    if (fragment_continuation) {
        flags |= LOG_COLLECTOR_PENDING_FLAG_CONTINUATION;
    }
    if (fragment_continues) {
        flags |= LOG_COLLECTOR_PENDING_FLAG_CONTINUES;
    }

    memcpy(&pending_buffer[*pending_len], content, content_len);
    *pending_len += content_len;
    pending_buffer[*pending_len] = '\0';
    (*pending_len)++;
    pending_buffer[*pending_len] = (char)flags;
    (*pending_len)++;
    pending_buffer[*pending_len] = '\0';

    return 0;
}

/*********************************************************************
*
*       _FlushPendingUntimedLines()
*
*  Function description
*    Deliver staged untimestamped lines with the supplied timestamp.
*/
static int _FlushPendingUntimedLines(LogSource_t source,
                                     char *pending_buffer,
                                     size_t *pending_len,
                                     uint64_t timestamp,
                                     uint64_t *sequence,
                                     LogCollector_Callback_t callback,
                                     void *user_data) {
    size_t pos;
    int    delivered;

    if (pending_buffer == NULL || pending_len == NULL || sequence == NULL) {
        return -1;
    }
    if (*pending_len == 0u) {
        return 0;
    }

    delivered = 0;
    pos = 0u;
    while (pos < *pending_len) {
        size_t line_start;
        size_t line_len;
        unsigned char flags;
        int    result;

        line_start = pos;
        while (pos < *pending_len && pending_buffer[pos] != '\0') {
            pos++;
        }
        line_len = pos - line_start;
        if (pos >= *pending_len) {
            return LOG_COLLECT_RESULT_INVALID_RTT;
        }
        pos++;
        if (pos >= *pending_len) {
            return LOG_COLLECT_RESULT_INVALID_RTT;
        }
        flags = (unsigned char)pending_buffer[pos];
        pos++;
        if (line_len == 0u) {
            continue;
        }

        result = _DeliverLogEntry(timestamp, source,
                                  &pending_buffer[line_start], line_len,
                                  sequence,
                                  (flags & LOG_COLLECTOR_PENDING_FLAG_CONTINUATION) != 0u,
                                  (flags & LOG_COLLECTOR_PENDING_FLAG_CONTINUES) != 0u,
                                  callback, user_data);
        if (result < 0) {
            return result;
        }
        delivered += result;
    }

    *pending_len = 0u;
    pending_buffer[0] = '\0';
    return delivered;
}

/*********************************************************************
*
*       _FlushPendingUntimedFallback()
*
*  Function description
*    Deliver staged untimestamped lines with a deterministic inferred
*    timestamp during shutdown cleanup.
*/
static int _FlushPendingUntimedFallback(LogSource_t source,
                                        char *pending_buffer,
                                        size_t *pending_len,
                                        uint64_t *timestamp,
                                        bool *timestamp_valid,
                                        uint64_t *sequence,
                                        LogCollector_Callback_t callback,
                                        void *user_data) {
    uint64_t fallback_timestamp;
    int      result;

    if (pending_buffer == NULL || pending_len == NULL ||
        timestamp == NULL || timestamp_valid == NULL ||
        sequence == NULL || callback == NULL) {
        return LOG_COLLECT_RESULT_INVALID_RTT;
    }
    if (*pending_len == 0u || *timestamp_valid) {
        return 0;
    }

    fallback_timestamp = _Collector_ReserveUntimedTimestamp(0u);
    result = _FlushPendingUntimedLines(source,
                                       pending_buffer,
                                       pending_len,
                                       fallback_timestamp,
                                       sequence,
                                       callback,
                                       user_data);
    if (result < 0) {
        return result;
    }

    *timestamp = fallback_timestamp;
    *timestamp_valid = true;
    return result;
}

/*********************************************************************
*
*       _GetPendingUntimedContentLen()
*
*  Function description
*    Return the payload byte count stored in the pending untimestamped buffer.
*/
static size_t _GetPendingUntimedContentLen(const char *pending_buffer,
                                           size_t pending_len) {
    size_t pos;
    size_t content_len;

    if (pending_buffer == NULL) {
        return 0u;
    }

    pos = 0u;
    content_len = 0u;
    while (pos < pending_len) {
        size_t line_start;

        line_start = pos;
        while (pos < pending_len && pending_buffer[pos] != '\0') {
            pos++;
        }
        if (pos >= pending_len) {
            return pending_len;
        }
        content_len += pos - line_start;
        pos++;
        if (pos >= pending_len) {
            return pending_len;
        }
        pos++;
    }

    return content_len;
}

/*********************************************************************
*
*       _Collector_ReportUnflushedUntimedLines()
*
*  Function description
*    Report leading untimestamped lines left after shutdown flushing.
*/
static void _Collector_ReportUnflushedUntimedLines(LogSource_t source,
                                                   const char *pending_buffer,
                                                   size_t *pending_len,
                                                   bool timestamp_valid) {
    const char *source_name;
    size_t      content_len;

    if (pending_buffer == NULL || pending_len == NULL || *pending_len == 0u || timestamp_valid) {
        return;
    }

    content_len = _GetPendingUntimedContentLen(pending_buffer, *pending_len);
    source_name = (source == LOG_SOURCE_LINUX) ? "Linux" : "RTOS";
    fprintf(stderr,
            "[LogCollector] %s: %zu leading untimestamped bytes were not rendered during cleanup\n",
            source_name, content_len);
    *pending_len = 0u;
}

/*********************************************************************
*
*       _ProcessLogLine()
*
*  Function description
*    Process a single log line and deliver via callback.
*    Strips a timestamp prefix when present. Leading lines without timestamps
*    are staged until the source reports its first timestamp. Later
*    untimestamped lines use an inferred timestamp that cannot move behind
*    timestamps already observed from other sources.
*
*  Parameters
*    line       Log line string (null-terminated)
*    line_len   Length of log line
*    source          Log source identifier
*    last_timestamp  Current source timestamp storage
*    timestamp_valid Current source timestamp validity
*    pending_buffer  Leading untimestamped line buffer
*    pending_size    Size of pending_buffer
*    pending_len     Current pending_buffer length
*    sequence   Source-local output sequence storage
*    fragment_continuation  Current content continues a previous line fragment
*    fragment_continues     Current content is followed by another fragment
*    callback   Callback function for delivering log entries
*    user_data  User-provided context pointer passed to callback
*
*  Return value
*    >= 0 Number of entries delivered
*    -1   Failed or ignored
*   LOG_COLLECT_RESULT_STOP  Callback requested stop
*   LOG_COLLECT_RESULT_PENDING_OVERFLOW Pending untimestamped data overflow
*/
static int _ProcessLogLine(const char *line, size_t line_len, LogSource_t source,
                           uint64_t *last_timestamp, bool *timestamp_valid,
                           char *pending_buffer, size_t pending_size,
                           size_t *pending_len, uint64_t *sequence,
                           bool fragment_continuation,
                           bool fragment_continues,
                           LogCollector_Callback_t callback, void *user_data) {
    uint64_t    entry_timestamp;
    const char *content;
    size_t      content_len;
    LogLineTimestampParseResult_t parse_result;
    uint64_t    parsed_timestamp;
    int         result;
    int         delivered;
    //
    // Skip empty lines
    //
    if (line_len == 0) {
        return -1;
    }
    if (callback == NULL || last_timestamp == NULL || timestamp_valid == NULL ||
        pending_buffer == NULL || pending_len == NULL || sequence == NULL) {
        return -1;
    }
    //
    // Parse an optional timestamp and locate content.
    //
    entry_timestamp = *last_timestamp;
    parsed_timestamp = entry_timestamp;
    content = LogLineParser_SkipHorizontalWhitespace(line);
    if (fragment_continuation) {
        parse_result = LOG_LINE_TIMESTAMP_PARSE_NONE;
        content = line;
    } else {
        parse_result = LogLineParser_ParseTimestampPrefix(line, &parsed_timestamp, &content);
    }
    if (parse_result == LOG_LINE_TIMESTAMP_PARSE_PRESENT) {
        entry_timestamp = parsed_timestamp;
    } else if (parse_result == LOG_LINE_TIMESTAMP_PARSE_EMPTY_CONTENT) {
        entry_timestamp = parsed_timestamp;
        _Collector_LockEmission();
        *last_timestamp = entry_timestamp;
        *timestamp_valid = true;
        _Collector_RecordObservedTimestamp(entry_timestamp);
        if (*pending_len == 0u) {
            _Collector_UnlockEmission();
            return 0;
        }
        result = _FlushPendingUntimedLines(source,
                                           pending_buffer,
                                           pending_len,
                                           entry_timestamp,
                                           sequence,
                                           callback,
                                           user_data);
        _Collector_UnlockEmission();
        return result;
    }
    content_len = strlen(content);
    //
    // Skip if content is empty.
    //
    if (content_len == 0) {
        return -1;
    }
    // Keep leading untimestamped lines behind the first source timestamp so
    // late-starting sources do not publish synthetic time before their real
    // timeline is known.
    //
    if (parse_result == LOG_LINE_TIMESTAMP_PARSE_NONE && !*timestamp_valid) {
        _Collector_LockEmission();
        result = _AppendPendingUntimedLine(source,
                                           pending_buffer,
                                           pending_size,
                                           pending_len,
                                           content,
                                           content_len,
                                           fragment_continuation,
                                           fragment_continues);
        _Collector_UnlockEmission();
        return result;
    }
    _Collector_LockEmission();
    if (parse_result == LOG_LINE_TIMESTAMP_PARSE_PRESENT) {
        *last_timestamp = entry_timestamp;
        *timestamp_valid = true;
        _Collector_RecordObservedTimestamp(entry_timestamp);
    }
    if (parse_result == LOG_LINE_TIMESTAMP_PARSE_NONE && !fragment_continuation) {
        entry_timestamp = _Collector_ReserveUntimedTimestamp(entry_timestamp);
        *last_timestamp = entry_timestamp;
        *timestamp_valid = true;
    }
    //
    // Deliver any leading untimestamped lines before the first timestamped line.
    //
    delivered = 0;
    if (parse_result == LOG_LINE_TIMESTAMP_PARSE_PRESENT && *pending_len > 0u) {
        result = _FlushPendingUntimedLines(source,
                                           pending_buffer,
                                           pending_len,
                                           entry_timestamp,
                                           sequence,
                                           callback,
                                           user_data);
        if (result < 0) {
            _Collector_UnlockEmission();
            return result;
        }
        delivered += result;
    }
    //
    // Deliver the current line.
    //
    result = _DeliverLogEntry(entry_timestamp,
                              source,
                              content,
                              content_len,
                              sequence,
                              fragment_continuation,
                              fragment_continues,
                              callback, user_data);
    if (result < 0) {
        _Collector_UnlockEmission();
        return result;
    }
    delivered += result;
    _Collector_UnlockEmission();
    return delivered;
}

/*********************************************************************
*
*       _ProcessBufferedLine()
*
*  Function description
*    Copy a complete buffered line into a local string and process it.
*
*  Parameters
*    line       Complete line bytes without the line ending
*    line_len   Length of line
*    source     Log source identifier
*    timestamp  Current source timestamp storage
*    timestamp_valid Current source timestamp validity
*    pending_buffer  Leading untimestamped line buffer
*    pending_size    Size of pending_buffer
*    pending_len     Current pending_buffer length
*    sequence   Source-local output sequence storage
*    fragment_continuation  Current content continues a previous line fragment
*    fragment_continues     Current content is followed by another fragment
*    callback   Callback function for delivering log entries
*    user_data  User-provided context pointer passed to callback
*
*  Return value
*    >= 0 Number of entries delivered
*   -1   Failed or ignored
*/
static int _ProcessBufferedLine(const char *line, size_t line_len, LogSource_t source,
                                uint64_t *timestamp, bool *timestamp_valid,
                                char *pending_buffer, size_t pending_size,
                                size_t *pending_len, uint64_t *sequence,
                                bool fragment_continuation,
                                bool fragment_continues,
                                LogCollector_Callback_t callback, void *user_data) {
    char   line_buffer[LOG_COLLECTOR_MAX_LINE_LEN];
    size_t copy_len;

    if (line == NULL || line_len == 0) {
        return -1;
    }

    copy_len = line_len;
    if (copy_len >= sizeof(line_buffer)) {
        copy_len = sizeof(line_buffer) - 1;
        copy_len = LogLineParser_AdjustUtf8Boundary(line, copy_len);
    }
    if (copy_len == 0) {
        return -1;
    }

    memcpy(line_buffer, line, copy_len);
    line_buffer[copy_len] = '\0';

    if (!fragment_continues) {
        copy_len = LogLineParser_TrimTrailingWhitespace(line_buffer);
    }
    if (copy_len == 0) {
        return -1;
    }

    return _ProcessLogLine(line_buffer, copy_len, source,
                           timestamp, timestamp_valid,
                           pending_buffer, pending_size, pending_len,
                           sequence,
                           fragment_continuation,
                           fragment_continues,
                           callback, user_data);
}

/*********************************************************************
*
*       _FlushPendingLine()
*
*  Function description
*    Process one pending line fragment at shutdown or cleanup.
*
*  Return value
*    0                               Success or no pending line
*    LOG_COLLECT_RESULT_INVALID_RTT  Invalid pending buffer state
*    LOG_COLLECT_RESULT_STOP         Callback requested stop
*    LOG_COLLECT_RESULT_PENDING_OVERFLOW Pending untimestamped data overflow
*/
static int _FlushPendingLine(LogSource_t source, char *buffer, size_t buffer_size,
                             size_t *buffer_len, bool *fragmenting,
                             uint64_t *timestamp, bool *timestamp_valid,
                             char *pending_buffer, size_t pending_size,
                             size_t *pending_len, uint64_t *sequence,
                             LogCollector_Callback_t callback, void *user_data) {
    int result;

    if (buffer == NULL || buffer_len == NULL || fragmenting == NULL ||
        timestamp == NULL || timestamp_valid == NULL ||
        pending_buffer == NULL || pending_len == NULL || sequence == NULL ||
        buffer_size < 2 || *buffer_len >= buffer_size) {
        return LOG_COLLECT_RESULT_INVALID_RTT;
    }

    if (*buffer_len == 0) {
        buffer[0] = '\0';
        *fragmenting = false;
        return _FlushPendingUntimedFallback(source,
                                            pending_buffer,
                                            pending_len,
                                            timestamp,
                                            timestamp_valid,
                                            sequence,
                                            callback,
                                            user_data);
    }

    buffer[*buffer_len] = '\0';
    result = _ProcessBufferedLine(buffer, *buffer_len, source,
                                  timestamp, timestamp_valid,
                                  pending_buffer, pending_size, pending_len,
                                  sequence,
                                  *fragmenting,
                                  false,
                                  callback, user_data);

    *buffer_len = 0;
    buffer[0] = '\0';
    *fragmenting = false;

    if (result == LOG_COLLECT_RESULT_STOP ||
        result == LOG_COLLECT_RESULT_PENDING_OVERFLOW) {
        return result;
    }

    return _FlushPendingUntimedFallback(source,
                                        pending_buffer,
                                        pending_len,
                                        timestamp,
                                        timestamp_valid,
                                        sequence,
                                        callback,
                                        user_data);
}

/*********************************************************************
*
*       _FlushPendingLines()
*
*  Function description
*    Flush pending line fragments for all sources.
*
*  Return value
*    >= 0                            Number of entries flushed
*    LOG_COLLECT_RESULT_INVALID_RTT  Invalid pending buffer state
*    LOG_COLLECT_RESULT_STOP         Callback requested stop
*    LOG_COLLECT_RESULT_PENDING_OVERFLOW Pending untimestamped data overflow
*/
static int _FlushPendingLines(LogCollector_Callback_t callback, void *user_data) {
    int result;
    int delivered;

    if (callback == NULL) {
        return 0;
    }

    delivered = 0;
    result = _FlushPendingLine(LOG_SOURCE_LINUX,
                               _collector_state.linux_buffer,
                               sizeof(_collector_state.linux_buffer),
                               &_collector_state.linux_buffer_len,
                               &_collector_state.linux_fragmenting_line,
                               &_collector_state.linux_last_timestamp,
                               &_collector_state.linux_last_timestamp_valid,
                               _collector_state.linux_pending_untimed,
                               _collector_state.pending_untimed_size,
                               &_collector_state.linux_pending_untimed_len,
                               &_collector_state.linux_sequence,
                               callback,
                               user_data);
    if (result < 0) {
        return result;
    }
    delivered += result;

    result = _FlushPendingLine(LOG_SOURCE_RTOS,
                               _collector_state.rtos_buffer,
                               sizeof(_collector_state.rtos_buffer),
                               &_collector_state.rtos_buffer_len,
                               &_collector_state.rtos_fragmenting_line,
                               &_collector_state.rtos_last_timestamp,
                               &_collector_state.rtos_last_timestamp_valid,
                               _collector_state.rtos_pending_untimed,
                               _collector_state.pending_untimed_size,
                               &_collector_state.rtos_pending_untimed_len,
                               &_collector_state.rtos_sequence,
                               callback,
                               user_data);
    if (result < 0) {
        return result;
    }
    delivered += result;

    return delivered;
}

/*********************************************************************
*
*       _CollectFromChannel()
*
*  Function description
*    Collect log entries from specified RTT channel.
*    Splits data by newlines and creates separate entries for each line.
*    Consumes bytes already recorded by CoreLogRecorder.
*
*  Parameters
*    channel      RTT channel number
*    source       Log source identifier
*    buffer       Pending line buffer
*    buffer_size  Size of pending line buffer
*    buffer_len   Current pending line length
*    fragmenting  Fragment state for a long line
*    timestamp    Current source timestamp storage
*    timestamp_valid Current source timestamp validity
*    pending_buffer  Leading untimestamped line buffer
*    pending_size    Size of pending_buffer
*    pending_len     Current pending_buffer length
*    sequence     Source-local output sequence storage
*    callback     Callback function for delivering log entries
*    user_data    User-provided context pointer passed to callback
*
*  Return value
*    >= 0  Number of entries collected
*    LOG_COLLECT_RESULT_NO_DATA      No data available
*    LOG_COLLECT_RESULT_INVALID_RTT  Recorder is not available
*    LOG_COLLECT_RESULT_STOP         Callback requested stop
*    LOG_COLLECT_RESULT_PENDING_OVERFLOW Pending untimestamped data overflow
*/
static int _CollectFromChannel(unsigned channel, LogSource_t source,
                               char *buffer, size_t buffer_size,
                               size_t *buffer_len, bool *fragmenting,
                               uint64_t *timestamp, bool *timestamp_valid,
                               char *pending_buffer, size_t pending_size,
                               size_t *pending_len, uint64_t *sequence,
                               LogCollector_Callback_t callback, void *user_data) {
    int         bytes_read;
    int         entries_collected;
    char        read_buffer[LOG_COLLECTOR_MAX_LINE_LEN];
    size_t      pos;
    //
    // Validate line buffer state
    //
    if (buffer == NULL || buffer_len == NULL || fragmenting == NULL ||
        timestamp == NULL || timestamp_valid == NULL ||
        pending_buffer == NULL || pending_len == NULL || sequence == NULL ||
        buffer_size < 2 || *buffer_len >= buffer_size) {
        return LOG_COLLECT_RESULT_INVALID_RTT;
    }
    bytes_read = CoreLogRecorder_ReadChannel(
        channel,
        read_buffer,
        sizeof(read_buffer) - 1
    );
    if (bytes_read < 0) {
        return LOG_COLLECT_RESULT_INVALID_RTT;
    }
    if (bytes_read == 0) {
        return LOG_COLLECT_RESULT_NO_DATA;
    }
    //
    // Null-terminate
    //
    read_buffer[bytes_read] = '\0';
    //
    // Append chunk bytes and process only complete lines.
    //
    entries_collected = 0;
    pos = 0;

    while (pos < (size_t)bytes_read) {
        if (read_buffer[pos] == '\n' || read_buffer[pos] == '\r') {
            int line_result;

            if (*buffer_len > 0u) {
                buffer[*buffer_len] = '\0';
                line_result = _ProcessBufferedLine(buffer, *buffer_len, source,
                                                   timestamp, timestamp_valid,
                                                   pending_buffer, pending_size,
                                                   pending_len,
                                                   sequence,
                                                   *fragmenting,
                                                   false,
                                                   callback, user_data);
                *buffer_len = 0;
                buffer[0] = '\0';
                if (line_result > 0) {
                    entries_collected += line_result;
                } else if (line_result == LOG_COLLECT_RESULT_STOP) {
                    return LOG_COLLECT_RESULT_STOP;
                } else if (line_result == LOG_COLLECT_RESULT_PENDING_OVERFLOW) {
                    return LOG_COLLECT_RESULT_PENDING_OVERFLOW;
                }
            }
            *fragmenting = false;

            if (read_buffer[pos] == '\r') {
                pos++;
            }
            if (pos < (size_t)bytes_read && read_buffer[pos] == '\n') {
                pos++;
            }
            continue;
        }

        if (*buffer_len >= buffer_size - 1) {
            int fragment_result;
            size_t emit_len;
            size_t remaining_len;

            emit_len = LogLineParser_AdjustUtf8Boundary(buffer, *buffer_len);
            if (emit_len == 0u) {
                emit_len = *buffer_len;
            }
            fragment_result = _ProcessBufferedLine(buffer, emit_len, source,
                                                   timestamp, timestamp_valid,
                                                   pending_buffer, pending_size,
                                                   pending_len,
                                                   sequence,
                                                   *fragmenting,
                                                   true,
                                                   callback, user_data);
            remaining_len = *buffer_len - emit_len;
            if (remaining_len > 0u) {
                memmove(buffer, buffer + emit_len, remaining_len);
            }
            *buffer_len = remaining_len;
            buffer[*buffer_len] = '\0';
            *fragmenting = true;
            if (fragment_result > 0) {
                entries_collected += fragment_result;
            } else if (fragment_result == LOG_COLLECT_RESULT_STOP) {
                return LOG_COLLECT_RESULT_STOP;
            } else if (fragment_result == LOG_COLLECT_RESULT_PENDING_OVERFLOW) {
                return LOG_COLLECT_RESULT_PENDING_OVERFLOW;
            } else if (fragment_result < 0) {
                return LOG_COLLECT_RESULT_INVALID_RTT;
            }
            continue;
        }

        buffer[*buffer_len] = read_buffer[pos];
        (*buffer_len)++;
        pos++;
    }

    buffer[*buffer_len] = '\0';

    return entries_collected;
}

/*********************************************************************
*
*       _LinuxCollectionThread()
*
*  Function description
*    Background thread for Linux log collection.
*    Consumes recorded Linux bytes and converts complete lines to entries.
*
*  Parameters
*    arg  Unused
*/
static void _LinuxCollectionThread(void *arg) {
    LogCollector_Callback_t callback;
    void                   *user_data;
    int                     result;

    (void)arg;
    callback  = _collector_state.callback;
    user_data = _collector_state.user_data;

    while (_Collector_IsRunning() && RTTBridge_IsRunning()) {
        result = _CollectFromChannel(
            _collector_state.config.linux_channel,
            LOG_SOURCE_LINUX,
            _collector_state.linux_buffer,
            sizeof(_collector_state.linux_buffer),
            &_collector_state.linux_buffer_len,
            &_collector_state.linux_fragmenting_line,
            &_collector_state.linux_last_timestamp,
            &_collector_state.linux_last_timestamp_valid,
            _collector_state.linux_pending_untimed,
            _collector_state.pending_untimed_size,
            &_collector_state.linux_pending_untimed_len,
            &_collector_state.linux_sequence,
            callback,
            user_data
        );
        if (result == LOG_COLLECT_RESULT_INVALID_RTT) {
            _Collector_ReportRTTError(LOG_SOURCE_LINUX, "recorder read");
            break;
        }
        if (result == LOG_COLLECT_RESULT_STOP) {
            _Collector_ReportCallbackStop(LOG_SOURCE_LINUX);
            break;
        }
        if (result == LOG_COLLECT_RESULT_PENDING_OVERFLOW) {
            break;
        }
        SYS_Sleep(_collector_state.config.poll_interval_ms);
    }
}

/*********************************************************************
*
*       _RTOSCollectionThread()
*
*  Function description
*    Background thread for RTOS log collection.
*    Consumes recorded RTOS bytes and converts complete lines to entries.
*
*  Parameters
*    arg  Unused
*/
static void _RTOSCollectionThread(void *arg) {
    LogCollector_Callback_t callback;
    void                   *user_data;
    int                     result;

    (void)arg;
    callback  = _collector_state.callback;
    user_data = _collector_state.user_data;

    while (_Collector_IsRunning() && RTTBridge_IsRunning()) {
        result = _CollectFromChannel(
            _collector_state.config.rtos_channel,
            LOG_SOURCE_RTOS,
            _collector_state.rtos_buffer,
            sizeof(_collector_state.rtos_buffer),
            &_collector_state.rtos_buffer_len,
            &_collector_state.rtos_fragmenting_line,
            &_collector_state.rtos_last_timestamp,
            &_collector_state.rtos_last_timestamp_valid,
            _collector_state.rtos_pending_untimed,
            _collector_state.pending_untimed_size,
            &_collector_state.rtos_pending_untimed_len,
            &_collector_state.rtos_sequence,
            callback,
            user_data
        );
        if (result == LOG_COLLECT_RESULT_INVALID_RTT) {
            _Collector_ReportRTTError(LOG_SOURCE_RTOS, "recorder read");
            break;
        }
        if (result == LOG_COLLECT_RESULT_STOP) {
            _Collector_ReportCallbackStop(LOG_SOURCE_RTOS);
            break;
        }
        if (result == LOG_COLLECT_RESULT_PENDING_OVERFLOW) {
            break;
        }
        SYS_Sleep(_collector_state.config.poll_interval_ms);
    }
}

/*********************************************************************
*
*       Public functions
*
**********************************************************************
*/

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
    int    result;
    size_t pending_untimed_size;
    //
    // Validate configuration
    //
    if (config == NULL) {
        return -1;
    }
    if (_Collector_IsRunning()) {
        return -1;
    }
    if (_collector_state.initialized) {
        LogCollector_Cleanup();
    }
    pending_untimed_size = LOG_COLLECTOR_DEFAULT_PENDING_UNTIMED_SIZE;
    if (pending_untimed_size < LOG_COLLECTOR_MIN_PENDING_UNTIMED_SIZE) {
        return -1;
    }
    //
    // Initialize state
    //
    memset(&_collector_state, 0, sizeof(_collector_state));
    memcpy(&_collector_state.config, config, sizeof(LogCollector_Config_t));
    if (_Collector_AllocatePendingBuffers(pending_untimed_size) != 0) {
        memset(&_collector_state, 0, sizeof(_collector_state));
        return -1;
    }
    //
    // Initialize locks
    //
    result = SYS_MutexInit(&_collector_state.lock);
    if (result != 0) {
        _Collector_FreePendingBuffers();
        memset(&_collector_state, 0, sizeof(_collector_state));
        return -1;
    }
    _collector_state.lock_initialized = true;
    result = SYS_MutexInit(&_collector_state.emit_lock);
    if (result != 0) {
        SYS_MutexDestroy(&_collector_state.lock);
        _collector_state.lock_initialized = false;
        _Collector_FreePendingBuffers();
        memset(&_collector_state, 0, sizeof(_collector_state));
        return -1;
    }
    _collector_state.emit_lock_initialized = true;
    //
    // Mark as initialized
    //
    _collector_state.initialized = true;

    return 0;
}

/*********************************************************************
*
*       LogCollector_Cleanup()
*
*  Function description
*    Cleanup log collector resources.
*/
void LogCollector_Cleanup(void) {
    //
    // Stop collection if running
    //
    if (_Collector_IsRunning() ||
        _collector_state.linux_thread_started ||
        _collector_state.rtos_thread_started) {
        LogCollector_Stop();
    }
    _Collector_ReportUnflushedUntimedLines(LOG_SOURCE_LINUX,
                                           _collector_state.linux_pending_untimed,
                                           &_collector_state.linux_pending_untimed_len,
                                           _collector_state.linux_last_timestamp_valid);
    _Collector_ReportUnflushedUntimedLines(LOG_SOURCE_RTOS,
                                           _collector_state.rtos_pending_untimed,
                                           &_collector_state.rtos_pending_untimed_len,
                                           _collector_state.rtos_last_timestamp_valid);
    _Collector_UnregisterConsumers();
    if (_collector_state.emit_lock_initialized) {
        SYS_MutexDestroy(&_collector_state.emit_lock);
        _collector_state.emit_lock_initialized = false;
    }
    if (_collector_state.lock_initialized) {
        SYS_MutexDestroy(&_collector_state.lock);
        _collector_state.lock_initialized = false;
    }
    _Collector_FreePendingBuffers();
    //
    // Reset state
    //
    memset(&_collector_state, 0, sizeof(_collector_state));
}

/*********************************************************************
*
*       LogCollector_Start()
*
*  Function description
*    Start log collection in background thread.
*    Collected entries are delivered via callback function.
*
*  Parameters
*    callback   Callback function for delivering log entries
*    user_data  User-provided context pointer passed to callback
*
*  Return value
*    0   Success
*   -1   Collector not initialized
*   -2   Failed to start collection thread
*/
int LogCollector_Start(LogCollector_Callback_t callback, void *user_data) {
    int result;
    //
    // Check initialization
    //
    if (!_collector_state.initialized) {
        return -1;
    }
    if (_Collector_HasFatalError()) {
        return -1;
    }
    if (callback == NULL) {
        return -1;
    }
    if (_Collector_IsRunning() ||
        _collector_state.linux_thread_started ||
        _collector_state.rtos_thread_started) {
        return -2;
    }
    if (_Collector_EnsureConsumersRegistered() != 0) {
        return -2;
    }
    //
    // Reset pending lines for a new background collection run
    //
    _collector_state.linux_buffer_len      = 0;
    _collector_state.rtos_buffer_len       = 0;
    _collector_state.linux_last_timestamp  = 0u;
    _collector_state.rtos_last_timestamp   = 0u;
    _collector_state.linux_last_timestamp_valid = false;
    _collector_state.rtos_last_timestamp_valid  = false;
    _collector_state.global_last_timestamp = 0u;
    _collector_state.global_last_timestamp_valid = false;
    _collector_state.linux_pending_untimed_len = 0u;
    _collector_state.rtos_pending_untimed_len  = 0u;
    _collector_state.linux_pending_untimed[0]  = '\0';
    _collector_state.rtos_pending_untimed[0]   = '\0';
    _collector_state.linux_fragmenting_line = false;
    _collector_state.rtos_fragmenting_line  = false;
    _collector_state.linux_sequence = 0u;
    _collector_state.rtos_sequence  = 0u;
    //
    // Save callback and user data
    //
    _collector_state.callback  = callback;
    _collector_state.user_data = user_data;
    _Collector_SetRunning(true);
    //
    // Start Linux collection thread
    //
    result = SYS_createThread(_LinuxCollectionThread, NULL,
                             &_collector_state.linux_thread);
    if (result < 0) {
        _Collector_SetRunning(false);
        _Collector_UnregisterConsumers();
        _collector_state.callback  = NULL;
        _collector_state.user_data = NULL;
        return -2;
    }
    _collector_state.linux_thread_started = true;
    //
    // Start RTOS collection thread
    //
    result = SYS_createThread(_RTOSCollectionThread, NULL,
                             &_collector_state.rtos_thread);
    if (result < 0) {
        _Collector_SetRunning(false);
        SYS_WaitThreadTerm(_collector_state.linux_thread);
        _collector_state.linux_thread_started = false;
        _Collector_UnregisterConsumers();
        _collector_state.callback  = NULL;
        _collector_state.user_data = NULL;
        return -2;
    }
    _collector_state.rtos_thread_started = true;

    return 0;
}

/*********************************************************************
*
*       LogCollector_Stop()
*
*  Function description
*    Stop log collection and wait for threads to finish.
*/
void LogCollector_Stop(void) {
    int result;

    //
    // Set running flag to false
    //
    _Collector_SetRunning(false);
    //
    // Wait for threads to finish
    //
    if (_collector_state.linux_thread_started) {
        SYS_WaitThreadTerm(_collector_state.linux_thread);
        _collector_state.linux_thread_started = false;
    }
    if (_collector_state.rtos_thread_started) {
        SYS_WaitThreadTerm(_collector_state.rtos_thread);
        _collector_state.rtos_thread_started = false;
    }
    result = _FlushPendingLines(_collector_state.callback, _collector_state.user_data);
    _Collector_ReportFlushError(result);
    _Collector_ReportUnflushedUntimedLines(LOG_SOURCE_LINUX,
                                           _collector_state.linux_pending_untimed,
                                           &_collector_state.linux_pending_untimed_len,
                                           _collector_state.linux_last_timestamp_valid);
    _Collector_ReportUnflushedUntimedLines(LOG_SOURCE_RTOS,
                                           _collector_state.rtos_pending_untimed,
                                           &_collector_state.rtos_pending_untimed_len,
                                           _collector_state.rtos_last_timestamp_valid);
    _Collector_UnregisterConsumers();
    _collector_state.callback  = NULL;
    _collector_state.user_data = NULL;
}

/*********************************************************************
*
*       LogCollector_Poll()
*
*  Function description
*    Poll for available log entries and call callback for each entry.
*    This is a synchronous, non-threaded collection mode.
*    Successful polls keep collector consumer registrations until
*    LogCollector_Stop() or LogCollector_Cleanup().
*
*  Parameters
*    callback   Callback function for delivering log entries
*    user_data  User-provided context pointer passed to callback
*
*  Return value
*    >= 0  Number of entries collected
*    -1    Collector not initialized, invalid callback, or collector is running
*    LOG_COLLECT_RESULT_INVALID_RTT  Recorder is not available or collector state is invalid
*    LOG_COLLECT_RESULT_STOP  Callback requested stop
*    LOG_COLLECT_RESULT_PENDING_OVERFLOW Pending untimestamped data overflow
*/
int LogCollector_Poll(LogCollector_Callback_t callback, void *user_data) {
    int count = 0;
    int result;
    //
    // Check initialization
    //
    if (!_collector_state.initialized) {
        return -1;
    }
    result = _Collector_GetFatalResult();
    if (result != 0) {
        return result;
    }
    if (callback == NULL) {
        return -1;
    }
    if (_Collector_IsRunning() ||
        _collector_state.linux_thread_started ||
        _collector_state.rtos_thread_started) {
        return -1;
    }
    if (_Collector_EnsureConsumersRegistered() != 0) {
        _Collector_SetFatalError();
        return LOG_COLLECT_RESULT_INVALID_RTT;
    }
    // Poll Linux channel
    //
    result = _CollectFromChannel(
        _collector_state.config.linux_channel,
        LOG_SOURCE_LINUX,
        _collector_state.linux_buffer,
        sizeof(_collector_state.linux_buffer),
        &_collector_state.linux_buffer_len,
        &_collector_state.linux_fragmenting_line,
        &_collector_state.linux_last_timestamp,
        &_collector_state.linux_last_timestamp_valid,
        _collector_state.linux_pending_untimed,
        _collector_state.pending_untimed_size,
        &_collector_state.linux_pending_untimed_len,
        &_collector_state.linux_sequence,
        callback,
        user_data
    );
    if (result == LOG_COLLECT_RESULT_INVALID_RTT) {
        return _Collector_StopCollectionWithResult(LOG_COLLECT_RESULT_INVALID_RTT);
    }
    if (result == LOG_COLLECT_RESULT_STOP) {
        return _Collector_StopCollectionWithResult(LOG_COLLECT_RESULT_STOP);
    }
    if (result == LOG_COLLECT_RESULT_PENDING_OVERFLOW) {
        return _Collector_StopCollectionWithResult(LOG_COLLECT_RESULT_PENDING_OVERFLOW);
    }
    if (result > 0) {
        count += result;
    }
    //
    // Poll RTOS channel
    //
    result = _CollectFromChannel(
        _collector_state.config.rtos_channel,
        LOG_SOURCE_RTOS,
        _collector_state.rtos_buffer,
        sizeof(_collector_state.rtos_buffer),
        &_collector_state.rtos_buffer_len,
        &_collector_state.rtos_fragmenting_line,
        &_collector_state.rtos_last_timestamp,
        &_collector_state.rtos_last_timestamp_valid,
        _collector_state.rtos_pending_untimed,
        _collector_state.pending_untimed_size,
        &_collector_state.rtos_pending_untimed_len,
        &_collector_state.rtos_sequence,
        callback,
        user_data
    );
    if (result == LOG_COLLECT_RESULT_INVALID_RTT) {
        return _Collector_StopCollectionWithResult(LOG_COLLECT_RESULT_INVALID_RTT);
    }
    if (result == LOG_COLLECT_RESULT_STOP) {
        return _Collector_StopCollectionWithResult(LOG_COLLECT_RESULT_STOP);
    }
    if (result == LOG_COLLECT_RESULT_PENDING_OVERFLOW) {
        return _Collector_StopCollectionWithResult(LOG_COLLECT_RESULT_PENDING_OVERFLOW);
    }
    if (result > 0) {
        count += result;
    }

    return count;
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
    return _Collector_IsRunning();
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
    return _Collector_HasFatalError();
}

/*************************** End of file ****************************/
