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
File    : LogMerger.h
Purpose : Log merger for time-ordered sorting of multiple log streams
---------------------------END-OF-HEADER------------------------------
*/

#ifndef TRACEHUB_LOGMERGER_H            // Guard against multiple inclusion
#define TRACEHUB_LOGMERGER_H

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include <stdint.h>
#include <stdbool.h>

#include "LogEntry.h"

#if defined(__cplusplus)         // Allow usage of this module from C++ files (disable name mangling)
extern "C" {     /* Make sure we have C-declarations in C++ programs */
#endif

/*********************************************************************
*
*       Defines
*
**********************************************************************
*/

/*********************************************************************
*
*       LOG_MERGER_DEFAULT_BUFFER_SIZE
*  Default size of internal sorting buffer.
*
*/
#ifndef LOG_MERGER_DEFAULT_BUFFER_SIZE
  #define LOG_MERGER_DEFAULT_BUFFER_SIZE   8192
#endif

/*********************************************************************
*
*       LOG_MERGER_DEFAULT_FLUSH_THRESHOLD
*  Default number of buffered entries that triggers a ready flush.
*
*/
#ifndef LOG_MERGER_DEFAULT_FLUSH_THRESHOLD
  #define LOG_MERGER_DEFAULT_FLUSH_THRESHOLD   4
#endif

/*********************************************************************
*
*       LOG_MERGER_DEFAULT_FLUSH_TIMEOUT_MS
*  Default maximum wait time before flushing ready entries.
*
*/
#ifndef LOG_MERGER_DEFAULT_FLUSH_TIMEOUT_MS
  #define LOG_MERGER_DEFAULT_FLUSH_TIMEOUT_MS  1000
#endif

/*********************************************************************
*
*       Types
*
**********************************************************************
*/

/*********************************************************************
*
*       LogMerger_Config_t
*
*  Description
*    Configuration structure for log merger.
*
*  Fields
*    buffer_size      Size of internal sorting buffer
*    required_source  Sources that must contribute ordering watermarks
*    log_enabled      Enable logging merged entries to file
*    log_prefix       Prefix for log file name (e.g., "swimlane")
*/
typedef struct {
    unsigned     buffer_size;
    bool         required_source[LOG_SOURCE_MAX];
    bool         log_enabled;
    const char  *log_prefix;
} LogMerger_Config_t;

/*********************************************************************
*
*       LogMerger_Output_t
*
*  Description
*    Callback function type for delivering sorted log entries.
*
*  Parameters
*    entry      Pointer to sorted LogEntry_t
*    user_data  User-provided context pointer
*
*  Return value
*    0   Continue merging
*   -1   Stop merging. The entry is consumed but is not recorded as
*        successfully delivered.
*
*  Ownership
*    The callback takes ownership of the entry and must call
*    LogEntry_Destroy(entry) regardless of the return value.
*/
typedef int (*LogMerger_Output_t)(LogEntry_t *entry, void *user_data);

/*********************************************************************
*
*       API functions
*
**********************************************************************
*/

int      LogMerger_Init            (LogMerger_Config_t *config);
void     LogMerger_Cleanup         (void);
int      LogMerger_Insert          (LogEntry_t *entry);
int      LogMerger_FlushReady      (LogMerger_Output_t output, void *user_data);
int      LogMerger_Flush           (LogMerger_Output_t output, void *user_data);
int      LogMerger_Process         (LogEntry_t *entry, LogMerger_Output_t output, void *user_data);
unsigned LogMerger_GetBufferedCount(void);
int      LogMerger_WriteEntry      (const LogEntry_t *entry);
bool     LogMerger_HasFileError    (void);

#if defined(__cplusplus)          // Allow usage of this module from C++ files (disable name mangling)
}                /* Make sure we have C-declarations in C++ programs */
#endif

#endif // end of include guard: TRACEHUB_LOGMERGER_H Avoid multiple inclusion

/*************************** End of file ****************************/
