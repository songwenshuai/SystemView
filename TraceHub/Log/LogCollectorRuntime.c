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
File    : LogCollectorRuntime.c
Purpose : LogCollector runtime collection paths
---------------------------END-OF-HEADER------------------------------
*/

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include <string.h>

#include "LogCollector_internal.h"
#include "CoreLogRecorder.h"
#include "RTTBridge.h"
#include "LogLineParser.h"

/*********************************************************************
*
*       Static functions
*
**********************************************************************
*/

/*********************************************************************
*
*       _CollectFromSource()
*
*  Function description
*    Collect log entries from one configured RTT source channel.
*
*  Return value
*    >= 0  Number of entries collected
*    LOG_COLLECT_RESULT_NO_DATA      No data available
*    LOG_COLLECT_RESULT_INVALID_RTT  Recorder is not available
*    LOG_COLLECT_RESULT_STOP         Callback requested stop
*    LOG_COLLECT_RESULT_PENDING_OVERFLOW Pending untimestamped data overflow
*/
static int _CollectFromSource(LogCollector_SourceState_t *source_state,
                              LogCollector_Callback_t callback,
                              void *user_data) {
    int    bytes_read;
    int    entries_collected;
    char   read_buffer[LOG_COLLECTOR_MAX_LINE_LEN];
    size_t pos;

    if (source_state == NULL ||
        source_state->pending_untimed == NULL ||
        source_state->line_buffer_len >= sizeof(source_state->line_buffer)) {
        return LOG_COLLECT_RESULT_INVALID_RTT;
    }

    bytes_read = CoreLogRecorder_ReadChannel(source_state->channel,
                                             read_buffer,
                                             sizeof(read_buffer) - 1u);
    if (bytes_read < 0) {
        return LOG_COLLECT_RESULT_INVALID_RTT;
    }
    if (bytes_read == 0) {
        return LOG_COLLECT_RESULT_NO_DATA;
    }

    read_buffer[bytes_read] = '\0';
    entries_collected = 0;
    pos = 0u;

    while (pos < (size_t)bytes_read) {
        if (read_buffer[pos] == '\n' || read_buffer[pos] == '\r') {
            int line_result;

            if (source_state->line_buffer_len > 0u) {
                source_state->line_buffer[source_state->line_buffer_len] = '\0';
                line_result = LogCollectorSource_ProcessBufferedLine(source_state,
                                                                     source_state->line_buffer,
                                                                     source_state->line_buffer_len,
                                                                     source_state->fragmenting_line,
                                                                     false,
                                                                     callback,
                                                                     user_data);
                source_state->line_buffer_len = 0u;
                source_state->line_buffer[0] = '\0';
                if (line_result > 0) {
                    entries_collected += line_result;
                } else if (line_result == LOG_COLLECT_RESULT_STOP) {
                    return LOG_COLLECT_RESULT_STOP;
                } else if (line_result == LOG_COLLECT_RESULT_PENDING_OVERFLOW) {
                    return LOG_COLLECT_RESULT_PENDING_OVERFLOW;
                }
            }
            source_state->fragmenting_line = false;

            if (read_buffer[pos] == '\r') {
                pos++;
            }
            if (pos < (size_t)bytes_read && read_buffer[pos] == '\n') {
                pos++;
            }
            continue;
        }

        if (source_state->line_buffer_len >= sizeof(source_state->line_buffer) - 1u) {
            int    fragment_result;
            size_t emit_len;
            size_t remaining_len;

            emit_len = LogLineParser_AdjustUtf8Boundary(source_state->line_buffer,
                                                        source_state->line_buffer_len);
            if (emit_len == 0u) {
                emit_len = source_state->line_buffer_len;
            }
            fragment_result = LogCollectorSource_ProcessBufferedLine(source_state,
                                                                     source_state->line_buffer,
                                                                     emit_len,
                                                                     source_state->fragmenting_line,
                                                                     true,
                                                                     callback,
                                                                     user_data);
            remaining_len = source_state->line_buffer_len - emit_len;
            if (remaining_len > 0u) {
                memmove(source_state->line_buffer,
                        source_state->line_buffer + emit_len,
                        remaining_len);
            }
            source_state->line_buffer_len = remaining_len;
            source_state->line_buffer[source_state->line_buffer_len] = '\0';
            source_state->fragmenting_line = true;
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

        source_state->line_buffer[source_state->line_buffer_len] = read_buffer[pos];
        source_state->line_buffer_len++;
        pos++;
    }

    source_state->line_buffer[source_state->line_buffer_len] = '\0';
    return entries_collected;
}

/*********************************************************************
*
*       _CollectionThread()
*
*  Function description
*    Background thread for one source log collection path.
*/
static void _CollectionThread(void *arg) {
    LogCollector_State_t       *state;
    LogCollector_SourceState_t *source_state;
    LogCollector_Callback_t     callback;
    void                       *user_data;
    int                         result;

    source_state = (LogCollector_SourceState_t *)arg;
    if (source_state == NULL) {
        return;
    }

    state = LogCollectorState_Get();
    callback = state->callback;
    user_data = state->user_data;

    while (LogCollectorState_IsRunning() && RTTBridge_IsRunning()) {
        result = _CollectFromSource(source_state, callback, user_data);
        if (result == LOG_COLLECT_RESULT_INVALID_RTT) {
            LogCollectorState_ReportRTTError(source_state->source, "recorder read");
            break;
        }
        if (result == LOG_COLLECT_RESULT_STOP) {
            LogCollectorState_ReportCallbackStop(source_state->source);
            break;
        }
        if (result == LOG_COLLECT_RESULT_PENDING_OVERFLOW) {
            break;
        }
        SYS_Sleep(state->config.poll_interval_ms);
    }
}

/*********************************************************************
*
*       _WaitStartedThreads()
*
*  Function description
*    Wait for all source threads that are currently marked as started.
*/
static void _WaitStartedThreads(LogCollector_State_t *state) {
    unsigned i;

    if (state == NULL) {
        return;
    }

    for (i = 0u; i < LOG_SOURCE_MAX; i++) {
        if (state->sources[i].thread_started) {
            SYS_WaitThreadTerm(state->sources[i].thread);
            state->sources[i].thread_started = false;
        }
    }
}

/*********************************************************************
*
*       _FlushPendingLines()
*
*  Function description
*    Flush pending line fragments for all sources.
*/
static int _FlushPendingLines(LogCollector_State_t *state) {
    int      result;
    int      delivered;
    unsigned i;

    if (state == NULL || state->callback == NULL) {
        return 0;
    }

    delivered = 0;
    for (i = 0u; i < LOG_SOURCE_MAX; i++) {
        result = LogCollectorSource_FlushPendingLine(&state->sources[i],
                                                     state->callback,
                                                     state->user_data);
        if (result < 0) {
            return result;
        }
        delivered += result;
    }
    return delivered;
}

/*********************************************************************
*
*       _ReportUnflushedSources()
*
*  Function description
*    Report any source that still owns leading untimestamped bytes.
*/
static void _ReportUnflushedSources(LogCollector_State_t *state) {
    unsigned i;

    if (state == NULL) {
        return;
    }

    for (i = 0u; i < LOG_SOURCE_MAX; i++) {
        LogCollectorSource_ReportUnflushedUntimedLines(&state->sources[i]);
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
*       LogCollector_Start()
*
*  Function description
*    Start log collection in background threads.
*/
int LogCollector_Start(LogCollector_Callback_t callback, void *user_data) {
    LogCollector_State_t *state;
    unsigned              i;
    int                   result;

    state = LogCollectorState_Get();
    if (!state->initialized) {
        return -1;
    }
    if (LogCollectorState_HasFatalError()) {
        return -1;
    }
    if (callback == NULL) {
        return -1;
    }
    if (LogCollectorState_IsRunning() || LogCollectorState_AnyThreadStarted()) {
        return -2;
    }
    if (LogCollectorState_EnsureConsumersRegistered() != 0) {
        return -2;
    }

    LogCollectorState_ResetSourcesForRun();
    state->callback = callback;
    state->user_data = user_data;
    LogCollectorState_SetRunning(true);

    for (i = 0u; i < LOG_SOURCE_MAX; i++) {
        result = SYS_createThread(_CollectionThread,
                                  &state->sources[i],
                                  &state->sources[i].thread);
        if (result < 0) {
            LogCollectorState_SetRunning(false);
            _WaitStartedThreads(state);
            LogCollectorState_UnregisterConsumers();
            state->callback = NULL;
            state->user_data = NULL;
            return -2;
        }
        state->sources[i].thread_started = true;
    }

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
    LogCollector_State_t *state;
    int                   result;

    state = LogCollectorState_Get();
    LogCollectorState_SetRunning(false);
    _WaitStartedThreads(state);
    result = _FlushPendingLines(state);
    LogCollectorState_ReportFlushError(result);
    _ReportUnflushedSources(state);
    LogCollectorState_UnregisterConsumers();
    state->callback = NULL;
    state->user_data = NULL;
}

/*********************************************************************
*
*       LogCollector_Poll()
*
*  Function description
*    Poll both sources synchronously for available log entries.
*/
int LogCollector_Poll(LogCollector_Callback_t callback, void *user_data) {
    LogCollector_State_t *state;
    int                   count;
    int                   result;
    unsigned              i;

    state = LogCollectorState_Get();
    if (!state->initialized) {
        return -1;
    }
    result = LogCollectorState_GetFatalResult();
    if (result != 0) {
        return result;
    }
    if (callback == NULL) {
        return -1;
    }
    if (LogCollectorState_IsRunning() || LogCollectorState_AnyThreadStarted()) {
        return -1;
    }
    if (LogCollectorState_EnsureConsumersRegistered() != 0) {
        LogCollectorState_SetFatalError();
        return LOG_COLLECT_RESULT_INVALID_RTT;
    }

    count = 0;
    for (i = 0u; i < LOG_SOURCE_MAX; i++) {
        result = _CollectFromSource(&state->sources[i], callback, user_data);
        if (result == LOG_COLLECT_RESULT_INVALID_RTT) {
            return LogCollectorState_StopCollectionWithResult(LOG_COLLECT_RESULT_INVALID_RTT);
        }
        if (result == LOG_COLLECT_RESULT_STOP) {
            return LogCollectorState_StopCollectionWithResult(LOG_COLLECT_RESULT_STOP);
        }
        if (result == LOG_COLLECT_RESULT_PENDING_OVERFLOW) {
            return LogCollectorState_StopCollectionWithResult(LOG_COLLECT_RESULT_PENDING_OVERFLOW);
        }
        if (result > 0) {
            count += result;
        }
    }

    return count;
}

/*************************** End of file ****************************/
