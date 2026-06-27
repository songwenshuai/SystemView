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
File    : CoreLogRecorder_internal.h
Purpose : Internal core log recorder state and function contracts
---------------------------END-OF-HEADER------------------------------
*/

#ifndef TRACEHUB_CORELOGRECORDER_INTERNAL_H
#define TRACEHUB_CORELOGRECORDER_INTERNAL_H

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "CoreLogRecorder.h"
#include "Log.h"
#include "SYS.h"

/*********************************************************************
*
*       Defines
*
**********************************************************************
*/

#define CORE_LOG_RECORDER_BUFFER_SIZE        CORE_LOG_RECORDER_MIN_QUEUE_SIZE
#define CORE_LOG_RECORDER_FLUSH_INTERVAL_US  100000u

/*********************************************************************
*
*       Types
*
**********************************************************************
*/

/*********************************************************************
*
*       CoreLogRecorder_SourceId_t
*
*  Description
*    Internal source index for the two owned core log sources.
*/
typedef enum {
    CORE_LOG_RECORDER_LINUX = 0,
    CORE_LOG_RECORDER_RTOS,
    CORE_LOG_RECORDER_NUM_SOURCES
} CoreLogRecorder_SourceId_t;

/*********************************************************************
*
*       CoreLogRecorder_Source_t
*
*  Description
*    Per-source file, consumer queue, and runtime statistics state.
*/
typedef struct {
    bool         enabled;
    unsigned     channel;
    const char  *name;
    FILE        *file;
    LOG_TextCleanState_t file_clean_state;
    bool         lock_initialized;
    SYS_Mutex    lock;
    bool         consumer_registered;
    bool         consumer_ever_registered;
    bool         seen;
    bool         file_error;
    bool         file_error_logged;
    bool         flush_pending;
    uint64_t     last_flush_time_us;
    uint64_t     bytes_recorded;
    uint64_t     bytes_consumed;
    uint64_t     consumer_undelivered_bytes;
    uint64_t     pending_undelivered_bytes;
    unsigned     consumer_capacity_failure_count;
    bool         queue_capacity_failure_pending;
    size_t       queue_size;
    size_t       queue_read;
    size_t       queue_used;
    char        *queue;
} CoreLogRecorder_Source_t;

/*********************************************************************
*
*       CoreLogRecorder_State_t
*
*  Description
*    Shared runtime state of the core log recorder module.
*/
typedef struct {
    CoreLogRecorder_Config_t config;
    bool                     initialized;
    bool                     running;
    bool                     fatal_error;
    bool                     lock_initialized;
    SYS_Mutex                lock;
    SYS_Thread               linux_thread;
    SYS_Thread               rtos_thread;
    bool                     linux_thread_started;
    bool                     rtos_thread_started;
    CoreLogRecorder_Source_t sources[CORE_LOG_RECORDER_NUM_SOURCES];
} CoreLogRecorder_State_t;

/*********************************************************************
*
*       Shared state
*
**********************************************************************
*/

extern CoreLogRecorder_State_t _recorder_state;

/*********************************************************************
*
*       State functions
*
**********************************************************************
*/

bool     _Recorder_IsRunning      (void);
void     _Recorder_SetRunning     (bool running);
void     _Recorder_SetFatalError  (void);
bool     _Recorder_HasFatalError  (void);
unsigned _Recorder_GetPollInterval(void);

/*********************************************************************
*
*       Source functions
*
**********************************************************************
*/

bool                     _Recorder_SourceIsEnabled     (CoreLogRecorder_SourceId_t source_id);
void                     _Recorder_RecordBytes         (CoreLogRecorder_SourceId_t source_id, const char *data, unsigned num_bytes);
void                     _Recorder_FlushAllSources     (void);
void                     _Recorder_CloseFiles          (void);
void                     _Recorder_DestroySourceLocks  (void);
void                     _Recorder_FreeSourceQueues    (void);
int                      _Recorder_InitSource          (CoreLogRecorder_SourceId_t source_id, unsigned channel, bool enabled, const char *name, const char *prefix);

/*********************************************************************
*
*       Statistics functions
*
**********************************************************************
*/

void _Recorder_ReportStats(void);

#endif /* TRACEHUB_CORELOGRECORDER_INTERNAL_H */

/*************************** End of file ****************************/
