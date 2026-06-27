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
File    : LogCollector.h
Purpose : Log collector for multiple sources (RTT and Linux logs)
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
