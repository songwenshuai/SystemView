/*********************************************************************
*                                                                    *
*                Copyright (C) 2023 xrTest Inc.                      *
*                      All rights reserved                           *
*                                                                    *
**********************************************************************
----------------------------------------------------------------------
File    : LogCollector.h
Purpose : Log collector for multiple sources (RTT and Linux logs)
Author  : songwenshuai <songwenshuai@gmail.com>
---------------------------END-OF-HEADER------------------------------
*/

#ifndef TRACEHUB_LOGCOLLECTOR_H            // Guard against multiple inclusion
#define TRACEHUB_LOGCOLLECTOR_H

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
*       LOG_COLLECTOR_MAX_LINE_LEN
*  Maximum length of a single log line.
*
*/
#ifndef LOG_COLLECTOR_MAX_LINE_LEN
  #define LOG_COLLECTOR_MAX_LINE_LEN   2048
#endif

/*********************************************************************
*
*       Types
*
**********************************************************************
*/

/*********************************************************************
*
*       LogCollector_Config_t
*
*  Description
*    Configuration structure for log collector.
*
*  Fields
*    linux_channel    RTT up-buffer channel number for Linux logs
*    rtos_channel     RTT up-buffer channel number for RTOS logs
*    poll_interval_ms Polling interval in milliseconds
*/
typedef struct {
    unsigned     linux_channel;
    unsigned     rtos_channel;
    unsigned     poll_interval_ms;
} LogCollector_Config_t;

/*********************************************************************
*
*       LogCollector_Callback_t
*
*  Description
*    Callback function type for delivering collected log entries.
*
*  Parameters
*    entry      Pointer to collected LogEntry_t
*    user_data  User-provided context pointer
*
*  Return value
*    0   Continue collection
*   -1   Stop collection
*
*  Ownership
*    The callback takes ownership of the entry and must call
*    LogEntry_Destroy(entry) regardless of the return value.
*/
typedef int (*LogCollector_Callback_t)(LogEntry_t *entry, void *user_data);

/*********************************************************************
*
*       API functions
*
**********************************************************************
*/

int  LogCollector_Init          (LogCollector_Config_t *config);
void LogCollector_Cleanup       (void);
int  LogCollector_Start         (LogCollector_Callback_t callback, void *user_data);
void LogCollector_Stop          (void);
/*********************************************************************
*
*       LogCollector_Poll()
*
*  Return value
*    >= 0  Number of entries collected
*    -1    Collector not initialized, invalid callback, or collector is running
*    -2    Recorder is not available or collector state is invalid
*    -3    Callback requested stop
*    -4    Leading untimestamped pending data exceeded its ordering buffer
*/
int  LogCollector_Poll          (LogCollector_Callback_t callback, void *user_data);
bool LogCollector_IsRunning     (void);
bool LogCollector_HasFatalError (void);

#if defined(__cplusplus)          // Allow usage of this module from C++ files (disable name mangling)
}                /* Make sure we have C-declarations in C++ programs */
#endif

#endif // end of include guard: TRACEHUB_LOGCOLLECTOR_H Avoid multiple inclusion

/*************************** End of file ****************************/
