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
File    : LogCollector_internal.h
Purpose : Internal LogCollector state and function contracts
---------------------------END-OF-HEADER------------------------------
*/

#ifndef TRACEHUB_LOGCOLLECTOR_INTERNAL_H
#define TRACEHUB_LOGCOLLECTOR_INTERNAL_H

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "LogCollector.h"
#include "SYS.h"

#if defined(__cplusplus)
extern "C" {
#endif

/*********************************************************************
*
*       Defines
*
**********************************************************************
*/

#define LOG_COLLECT_RESULT_NO_DATA          (-1)
#define LOG_COLLECT_RESULT_INVALID_RTT      (-2)
#define LOG_COLLECT_RESULT_STOP             (-3)
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
*       LogCollector_SourceState_t
*
*  Description
*    Runtime state for one collected RTT text source.
*/
typedef struct {
    LogSource_t source;
    unsigned    channel;
    SYS_Thread  thread;
    bool        thread_started;
    bool        consumer_registered;
    char        line_buffer[LOG_COLLECTOR_MAX_LINE_LEN];
    size_t      line_buffer_len;
    uint64_t    last_timestamp;
    bool        last_timestamp_valid;
    char       *pending_untimed;
    size_t      pending_untimed_size;
    size_t      pending_untimed_len;
    bool        fragmenting_line;
    uint64_t    sequence;
} LogCollector_SourceState_t;

/*********************************************************************
*
*       LogCollector_State_t
*
*  Description
*    Runtime state of the log collector.
*/
typedef struct {
    LogCollector_Config_t      config;
    bool                       initialized;
    bool                       running;
    bool                       lock_initialized;
    bool                       emit_lock_initialized;
    LogCollector_Callback_t    callback;
    void                      *user_data;
    SYS_Mutex                  lock;
    SYS_Mutex                  emit_lock;
    LogCollector_SourceState_t sources[LOG_SOURCE_MAX];
    size_t                     pending_untimed_size;
    uint64_t                   global_last_timestamp;
    bool                       global_last_timestamp_valid;
    bool                       fatal_error;
    int                        fatal_result;
} LogCollector_State_t;

/*********************************************************************
*
*       Shared state
*
**********************************************************************
*/

extern LogCollector_State_t _collector_state;

/*********************************************************************
*
*       State functions
*
**********************************************************************
*/

LogCollector_State_t *LogCollectorState_Get                      (void);
const char           *LogCollectorState_GetSourceName             (LogSource_t source);
int                   LogCollectorState_Init                     (const LogCollector_Config_t *config);
void                  LogCollectorState_Destroy                  (void);
void                  LogCollectorState_ResetStorage             (void);
void                  LogCollectorState_ResetSourcesForRun       (void);
bool                  LogCollectorState_AnyThreadStarted         (void);
bool                  LogCollectorState_IsRunning                (void);
void                  LogCollectorState_SetRunning               (bool running);
void                  LogCollectorState_LockEmission             (void);
void                  LogCollectorState_UnlockEmission           (void);
void                  LogCollectorState_RecordObservedTimestamp  (uint64_t timestamp);
uint64_t              LogCollectorState_ReserveUntimedTimestamp  (uint64_t requested_timestamp);
bool                  LogCollectorState_HasFatalError            (void);
int                   LogCollectorState_GetFatalResult           (void);
void                  LogCollectorState_SetFatalErrorResult      (int result);
void                  LogCollectorState_SetFatalError            (void);
bool                  LogCollectorState_EnterFatalStateWithResult(int result);
bool                  LogCollectorState_EnterFatalState          (void);
void                  LogCollectorState_ReportRTTError           (LogSource_t source, const char *operation);
void                  LogCollectorState_ReportCallbackStop       (LogSource_t source);
void                  LogCollectorState_ReportFlushError         (int result);
int                   LogCollectorState_EnsureConsumersRegistered(void);
void                  LogCollectorState_UnregisterConsumers      (void);
int                   LogCollectorState_StopCollectionWithResult (int result);
void                  LogCollectorState_RecordPendingOverflow    (LogSource_t source, size_t limit);

/*********************************************************************
*
*       Source functions
*
**********************************************************************
*/

void LogCollectorSource_Init                   (LogCollector_SourceState_t *source_state, LogSource_t source, unsigned channel);
void LogCollectorSource_ResetForRun            (LogCollector_SourceState_t *source_state);
int  LogCollectorSource_AppendPendingUntimedLine(LogCollector_SourceState_t *source_state, const char *content, size_t content_len, bool fragment_continuation, bool fragment_continues);
int  LogCollectorSource_FlushPendingUntimedFallback(LogCollector_SourceState_t *source_state, LogCollector_Callback_t callback, void *user_data);
int  LogCollectorSource_ProcessLogLine         (LogCollector_SourceState_t *source_state, const char *line, size_t line_len, bool fragment_continuation, bool fragment_continues, LogCollector_Callback_t callback, void *user_data);
int  LogCollectorSource_ProcessBufferedLine    (LogCollector_SourceState_t *source_state, const char *line, size_t line_len, bool fragment_continuation, bool fragment_continues, LogCollector_Callback_t callback, void *user_data);
int  LogCollectorSource_FlushPendingLine       (LogCollector_SourceState_t *source_state, LogCollector_Callback_t callback, void *user_data);
void LogCollectorSource_ReportUnflushedUntimedLines(LogCollector_SourceState_t *source_state);

#if defined(__cplusplus)
}
#endif

#endif /* TRACEHUB_LOGCOLLECTOR_INTERNAL_H */

/*************************** End of file ****************************/
