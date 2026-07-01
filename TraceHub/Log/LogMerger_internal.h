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
File    : LogMerger_internal.h
Purpose : Internal log merger state and function contracts
---------------------------END-OF-HEADER------------------------------
*/

#ifndef TRACEHUB_LOGMERGER_INTERNAL_H
#define TRACEHUB_LOGMERGER_INTERNAL_H

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "LogMerger.h"
#include "Log.h"
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
    unsigned             flush_threshold;
    unsigned             flush_timeout_ms;
    const char          *source_label[LOG_SOURCE_MAX];
    uint64_t             last_delivered_timestamp;
    LogSource_t          last_delivered_source;
    uint64_t             last_delivered_sequence;
    bool                 last_delivered_valid;
    FILE                *log_file;
    LOG_TextCleanState_t log_clean_state[LOG_SOURCE_MAX];
    SYS_Mutex            operation_mutex;
    bool                 operation_mutex_initialized;
    SYS_Mutex            mutex;
    bool                 mutex_initialized;
    bool                 log_file_error;
} LogMerger_State_t;

/*********************************************************************
*
*       Shared state
*
**********************************************************************
*/

extern LogMerger_State_t _merger_state;

/*********************************************************************
*
*       Buffer functions
*
**********************************************************************
*/

int _Merger_InsertEntryAndUpdateWatermark(LogEntry_t *entry);
int _Merger_DetachBufferedEntries        (LogEntry_t ***entries, unsigned *count);
int _Merger_DetachReadyEntriesLimited    (LogEntry_t ***entries, unsigned *count, const LogEntry_t *limit_entry);
int _Merger_DetachReadyEntries           (LogEntry_t ***entries, unsigned *count);

/*********************************************************************
*
*       Watermark functions
*
**********************************************************************
*/

bool _Merger_EntrySourceIsRequired  (const LogEntry_t *entry);
bool _Merger_GetReadyWatermark      (uint64_t *watermark);
bool _Merger_EntryIsBeforeLastDelivered(const LogEntry_t *entry);
int  _Merger_UpdateSourceWatermark  (const LogEntry_t *entry);
void _Merger_RecordDeliveredKey     (uint64_t timestamp, LogSource_t source, uint64_t sequence);

/*********************************************************************
*
*       Output functions
*
**********************************************************************
*/

int      _Merger_WaitForCapacityReadyEntries(LogEntry_t ***entries, unsigned *count, const LogEntry_t *limit_entry);
unsigned _Merger_DeliverDetachedEntries      (LogEntry_t **entries, unsigned count, LogMerger_Output_t output, void *user_data, bool *stopped);

/*********************************************************************
*
*       File functions
*
**********************************************************************
*/

void _Merger_InitLogFileState       (void);
int  _Merger_OpenLogFile            (const LogMerger_Config_t *config);
void _Merger_CloseLogFileLocked     (void);
void _Merger_ReportLogFileErrorLocked(const char *operation, int error_code);

#if defined(__cplusplus)
}
#endif

#endif /* TRACEHUB_LOGMERGER_INTERNAL_H */

/*************************** End of file ****************************/
