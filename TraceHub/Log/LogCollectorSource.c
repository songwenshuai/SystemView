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
File    : LogCollectorSource.c
Purpose : LogCollector per-source line processing
---------------------------END-OF-HEADER------------------------------
*/

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include <stdio.h>
#include <string.h>

#include "LogCollector_internal.h"
#include "LogLineParser.h"

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
static int _DeliverLogEntry(uint64_t timestamp,
                            LogCollector_SourceState_t *source_state,
                            const char *content,
                            size_t content_len,
                            bool fragment_continuation,
                            bool fragment_continues,
                            LogCollector_Callback_t callback,
                            void *user_data) {
    size_t offset;
    int    delivered;

    if (source_state == NULL || content == NULL || content_len == 0u || callback == NULL) {
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
                                  source_state->source,
                                  source_state->sequence,
                                  current_continuation,
                                  current_continues,
                                  content + offset,
                                  fragment_len);
        if (entry == NULL) {
            return -1;
        }
        source_state->sequence++;

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
*       _FlushPendingUntimedLines()
*
*  Function description
*    Deliver staged untimestamped lines with the supplied timestamp.
*/
static int _FlushPendingUntimedLines(LogCollector_SourceState_t *source_state,
                                     uint64_t timestamp,
                                     LogCollector_Callback_t callback,
                                     void *user_data) {
    size_t pos;
    int    delivered;

    if (source_state == NULL || source_state->pending_untimed == NULL) {
        return -1;
    }
    if (source_state->pending_untimed_len == 0u) {
        return 0;
    }

    delivered = 0;
    pos = 0u;
    while (pos < source_state->pending_untimed_len) {
        size_t        line_start;
        size_t        line_len;
        unsigned char flags;
        int           result;

        line_start = pos;
        while (pos < source_state->pending_untimed_len &&
               source_state->pending_untimed[pos] != '\0') {
            pos++;
        }
        line_len = pos - line_start;
        if (pos >= source_state->pending_untimed_len) {
            return LOG_COLLECT_RESULT_INVALID_RTT;
        }
        pos++;
        if (pos >= source_state->pending_untimed_len) {
            return LOG_COLLECT_RESULT_INVALID_RTT;
        }
        flags = (unsigned char)source_state->pending_untimed[pos];
        pos++;
        if (line_len == 0u) {
            continue;
        }

        result = _DeliverLogEntry(timestamp,
                                  source_state,
                                  &source_state->pending_untimed[line_start],
                                  line_len,
                                  (flags & LOG_COLLECTOR_PENDING_FLAG_CONTINUATION) != 0u,
                                  (flags & LOG_COLLECTOR_PENDING_FLAG_CONTINUES) != 0u,
                                  callback,
                                  user_data);
        if (result < 0) {
            return result;
        }
        delivered += result;
    }

    source_state->pending_untimed_len = 0u;
    source_state->pending_untimed[0] = '\0';
    return delivered;
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
*       Public functions
*
**********************************************************************
*/

/*********************************************************************
*
*       LogCollectorSource_Init()
*
*  Function description
*    Initialize one source state with its semantic source and RTT channel.
*/
void LogCollectorSource_Init(LogCollector_SourceState_t *source_state,
                             LogSource_t source,
                             unsigned channel) {
    if (source_state == NULL) {
        return;
    }

    memset(source_state, 0, sizeof(*source_state));
    source_state->source = source;
    source_state->channel = channel;
}

/*********************************************************************
*
*       LogCollectorSource_ResetForRun()
*
*  Function description
*    Reset one source pipeline before a new collection run.
*/
void LogCollectorSource_ResetForRun(LogCollector_SourceState_t *source_state) {
    if (source_state == NULL) {
        return;
    }

    source_state->line_buffer_len = 0u;
    source_state->line_buffer[0] = '\0';
    source_state->last_timestamp = 0u;
    source_state->last_timestamp_valid = false;
    source_state->pending_untimed_len = 0u;
    if (source_state->pending_untimed != NULL) {
        source_state->pending_untimed[0] = '\0';
    }
    source_state->fragmenting_line = false;
    source_state->sequence = 0u;
}

/*********************************************************************
*
*       LogCollectorSource_AppendPendingUntimedLine()
*
*  Function description
*    Stage an untimestamped line until the source timeline is known.
*/
int LogCollectorSource_AppendPendingUntimedLine(LogCollector_SourceState_t *source_state,
                                                const char *content,
                                                size_t content_len,
                                                bool fragment_continuation,
                                                bool fragment_continues) {
    unsigned char flags;

    if (source_state == NULL ||
        source_state->pending_untimed == NULL ||
        content == NULL ||
        content_len == 0u ||
        source_state->pending_untimed_size < LOG_COLLECTOR_PENDING_RECORD_OVERHEAD) {
        return -1;
    }

    if (source_state->pending_untimed_len >
            source_state->pending_untimed_size - LOG_COLLECTOR_PENDING_RECORD_OVERHEAD ||
        content_len >
            source_state->pending_untimed_size - source_state->pending_untimed_len -
            LOG_COLLECTOR_PENDING_RECORD_OVERHEAD) {
        LogCollectorState_RecordPendingOverflow(source_state->source,
                                                source_state->pending_untimed_size - 1u);
        return LOG_COLLECT_RESULT_PENDING_OVERFLOW;
    }

    flags = 0u;
    if (fragment_continuation) {
        flags |= LOG_COLLECTOR_PENDING_FLAG_CONTINUATION;
    }
    if (fragment_continues) {
        flags |= LOG_COLLECTOR_PENDING_FLAG_CONTINUES;
    }

    memcpy(&source_state->pending_untimed[source_state->pending_untimed_len],
           content,
           content_len);
    source_state->pending_untimed_len += content_len;
    source_state->pending_untimed[source_state->pending_untimed_len] = '\0';
    source_state->pending_untimed_len++;
    source_state->pending_untimed[source_state->pending_untimed_len] = (char)flags;
    source_state->pending_untimed_len++;
    source_state->pending_untimed[source_state->pending_untimed_len] = '\0';
    return 0;
}

/*********************************************************************
*
*       LogCollectorSource_FlushPendingUntimedFallback()
*
*  Function description
*    Deliver staged untimestamped lines with a deterministic inferred
*    timestamp during shutdown cleanup.
*/
int LogCollectorSource_FlushPendingUntimedFallback(LogCollector_SourceState_t *source_state,
                                                   LogCollector_Callback_t callback,
                                                   void *user_data) {
    uint64_t fallback_timestamp;
    int      result;

    if (source_state == NULL || callback == NULL) {
        return LOG_COLLECT_RESULT_INVALID_RTT;
    }
    if (source_state->pending_untimed_len == 0u || source_state->last_timestamp_valid) {
        return 0;
    }

    fallback_timestamp = LogCollectorState_ReserveUntimedTimestamp(0u);
    result = _FlushPendingUntimedLines(source_state,
                                       fallback_timestamp,
                                       callback,
                                       user_data);
    if (result < 0) {
        return result;
    }

    source_state->last_timestamp = fallback_timestamp;
    source_state->last_timestamp_valid = true;
    return result;
}

/*********************************************************************
*
*       LogCollectorSource_ProcessLogLine()
*
*  Function description
*    Process a single log line and deliver via callback.
*/
int LogCollectorSource_ProcessLogLine(LogCollector_SourceState_t *source_state,
                                      const char *line,
                                      size_t line_len,
                                      bool fragment_continuation,
                                      bool fragment_continues,
                                      LogCollector_Callback_t callback,
                                      void *user_data) {
    uint64_t                        entry_timestamp;
    const char                     *content;
    size_t                          content_len;
    LogLineTimestampParseResult_t   parse_result;
    uint64_t                        parsed_timestamp;
    int                             result;
    int                             delivered;

    if (line_len == 0u) {
        return -1;
    }
    if (source_state == NULL ||
        callback == NULL ||
        source_state->pending_untimed == NULL ||
        line == NULL) {
        return -1;
    }

    entry_timestamp = source_state->last_timestamp;
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
        LogCollectorState_LockEmission();
        source_state->last_timestamp = entry_timestamp;
        source_state->last_timestamp_valid = true;
        LogCollectorState_RecordObservedTimestamp(entry_timestamp);
        if (source_state->pending_untimed_len == 0u) {
            LogCollectorState_UnlockEmission();
            return 0;
        }
        result = _FlushPendingUntimedLines(source_state,
                                           entry_timestamp,
                                           callback,
                                           user_data);
        LogCollectorState_UnlockEmission();
        return result;
    }

    content_len = strlen(content);
    if (content_len == 0u) {
        return -1;
    }

    if (parse_result == LOG_LINE_TIMESTAMP_PARSE_NONE &&
        !source_state->last_timestamp_valid) {
        LogCollectorState_LockEmission();
        result = LogCollectorSource_AppendPendingUntimedLine(source_state,
                                                            content,
                                                            content_len,
                                                            fragment_continuation,
                                                            fragment_continues);
        LogCollectorState_UnlockEmission();
        return result;
    }

    LogCollectorState_LockEmission();
    if (parse_result == LOG_LINE_TIMESTAMP_PARSE_PRESENT) {
        source_state->last_timestamp = entry_timestamp;
        source_state->last_timestamp_valid = true;
        LogCollectorState_RecordObservedTimestamp(entry_timestamp);
    }
    if (parse_result == LOG_LINE_TIMESTAMP_PARSE_NONE && !fragment_continuation) {
        entry_timestamp = LogCollectorState_ReserveUntimedTimestamp(entry_timestamp);
        source_state->last_timestamp = entry_timestamp;
        source_state->last_timestamp_valid = true;
    }

    delivered = 0;
    if (parse_result == LOG_LINE_TIMESTAMP_PARSE_PRESENT &&
        source_state->pending_untimed_len > 0u) {
        result = _FlushPendingUntimedLines(source_state,
                                           entry_timestamp,
                                           callback,
                                           user_data);
        if (result < 0) {
            LogCollectorState_UnlockEmission();
            return result;
        }
        delivered += result;
    }

    result = _DeliverLogEntry(entry_timestamp,
                              source_state,
                              content,
                              content_len,
                              fragment_continuation,
                              fragment_continues,
                              callback,
                              user_data);
    if (result < 0) {
        LogCollectorState_UnlockEmission();
        return result;
    }
    delivered += result;
    LogCollectorState_UnlockEmission();
    return delivered;
}

/*********************************************************************
*
*       LogCollectorSource_ProcessBufferedLine()
*
*  Function description
*    Copy a complete buffered line into a local string and process it.
*/
int LogCollectorSource_ProcessBufferedLine(LogCollector_SourceState_t *source_state,
                                           const char *line,
                                           size_t line_len,
                                           bool fragment_continuation,
                                           bool fragment_continues,
                                           LogCollector_Callback_t callback,
                                           void *user_data) {
    char   line_buffer[LOG_COLLECTOR_MAX_LINE_LEN];
    size_t copy_len;

    if (source_state == NULL || line == NULL || line_len == 0u) {
        return -1;
    }

    copy_len = line_len;
    if (copy_len >= sizeof(line_buffer)) {
        copy_len = sizeof(line_buffer) - 1u;
        copy_len = LogLineParser_AdjustUtf8Boundary(line, copy_len);
    }
    if (copy_len == 0u) {
        return -1;
    }

    memcpy(line_buffer, line, copy_len);
    line_buffer[copy_len] = '\0';

    if (!fragment_continues) {
        copy_len = LogLineParser_TrimTrailingWhitespace(line_buffer);
    }
    if (copy_len == 0u) {
        return -1;
    }

    return LogCollectorSource_ProcessLogLine(source_state,
                                             line_buffer,
                                             copy_len,
                                             fragment_continuation,
                                             fragment_continues,
                                             callback,
                                             user_data);
}

/*********************************************************************
*
*       LogCollectorSource_FlushPendingLine()
*
*  Function description
*    Process one pending line fragment at shutdown or cleanup.
*/
int LogCollectorSource_FlushPendingLine(LogCollector_SourceState_t *source_state,
                                        LogCollector_Callback_t callback,
                                        void *user_data) {
    int result;

    if (source_state == NULL ||
        source_state->pending_untimed == NULL ||
        sizeof(source_state->line_buffer) < 2u ||
        source_state->line_buffer_len >= sizeof(source_state->line_buffer)) {
        return LOG_COLLECT_RESULT_INVALID_RTT;
    }

    if (source_state->line_buffer_len == 0u) {
        source_state->line_buffer[0] = '\0';
        source_state->fragmenting_line = false;
        return LogCollectorSource_FlushPendingUntimedFallback(source_state,
                                                             callback,
                                                             user_data);
    }

    source_state->line_buffer[source_state->line_buffer_len] = '\0';
    result = LogCollectorSource_ProcessBufferedLine(source_state,
                                                    source_state->line_buffer,
                                                    source_state->line_buffer_len,
                                                    source_state->fragmenting_line,
                                                    false,
                                                    callback,
                                                    user_data);

    source_state->line_buffer_len = 0u;
    source_state->line_buffer[0] = '\0';
    source_state->fragmenting_line = false;

    if (result == LOG_COLLECT_RESULT_STOP ||
        result == LOG_COLLECT_RESULT_PENDING_OVERFLOW) {
        return result;
    }

    return LogCollectorSource_FlushPendingUntimedFallback(source_state,
                                                         callback,
                                                         user_data);
}

/*********************************************************************
*
*       LogCollectorSource_ReportUnflushedUntimedLines()
*
*  Function description
*    Report leading untimestamped lines left after shutdown flushing.
*/
void LogCollectorSource_ReportUnflushedUntimedLines(LogCollector_SourceState_t *source_state) {
    size_t content_len;

    if (source_state == NULL ||
        source_state->pending_untimed == NULL ||
        source_state->pending_untimed_len == 0u ||
        source_state->last_timestamp_valid) {
        return;
    }

    content_len = _GetPendingUntimedContentLen(source_state->pending_untimed,
                                               source_state->pending_untimed_len);
    fprintf(stderr,
            "[LogCollector] %s: %zu leading untimestamped bytes were not rendered during cleanup\n",
            _Collector_GetSourceName(source_state->source),
            content_len);
    source_state->pending_untimed_len = 0u;
}

/*************************** End of file ****************************/
