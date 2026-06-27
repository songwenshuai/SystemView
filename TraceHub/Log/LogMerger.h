/*********************************************************************
*                                                                    *
*                Copyright (C) 2023 xrTest Inc.                      *
*                      All rights reserved                           *
*                                                                    *
**********************************************************************
----------------------------------------------------------------------
File    : LogMerger.h
Purpose : Log merger for time-ordered sorting of multiple log streams
Author  : songwenshuai <songwenshuai@gmail.com>
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
*    flush_threshold  Flush when buffer reaches this size
*    flush_timeout_ms Flush when buffered entries wait past this timeout; 0 disables timeout flushing
*    required_source  Sources that must contribute ordering watermarks
*    log_enabled      Enable logging merged entries to file
*    log_prefix       Prefix for log file name (e.g., "swimlane")
*/
typedef struct {
    unsigned     buffer_size;
    unsigned     flush_threshold;
    unsigned     flush_timeout_ms;
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

int      LogMerger_Init             (LogMerger_Config_t *config);
void     LogMerger_Cleanup          (void);
int      LogMerger_Insert           (LogEntry_t *entry);
/*********************************************************************
*
*       LogMerger_FlushReady()
*
*  Function description
*    Flush entries that are safe to deliver according to required source
*    timestamp watermarks.
*/
int      LogMerger_FlushReady       (LogMerger_Output_t output, void *user_data);
/*********************************************************************
*
*       LogMerger_Flush()
*
*  Function description
*    Force flush all buffered entries. Use during shutdown after
*    collectors have stopped.
*/
int      LogMerger_Flush            (LogMerger_Output_t output, void *user_data);
/* LogMerger_Process() takes ownership of entry regardless of return value. */
int      LogMerger_Process          (LogEntry_t *entry, LogMerger_Output_t output, void *user_data);
unsigned LogMerger_GetBufferedCount (void);
/*********************************************************************
*
*       LogMerger_WriteEntry()
*
*  Function description
*    Persist a merged entry to the merger-owned log file when logging is
*    enabled.
*
*  Return value
*    0   Success or logging disabled
*   -1   Log file write failed
*/
int      LogMerger_WriteEntry       (const LogEntry_t *entry);
/*********************************************************************
*
*       LogMerger_HasFileError()
*
*  Function description
*    Check whether the merger log file encountered a persistence error.
*/
bool     LogMerger_HasFileError     (void);

#if defined(__cplusplus)          // Allow usage of this module from C++ files (disable name mangling)
}                /* Make sure we have C-declarations in C++ programs */
#endif

#endif // end of include guard: TRACEHUB_LOGMERGER_H Avoid multiple inclusion

/*************************** End of file ****************************/
