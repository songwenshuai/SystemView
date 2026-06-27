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
File    : LogEntry.h
Purpose : Log entry data structure for swimlane display
---------------------------END-OF-HEADER------------------------------
*/

#ifndef TRACEHUB_LOGENTRY_H            // Guard against multiple inclusion
#define TRACEHUB_LOGENTRY_H

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

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
*       LOG_ENTRY_MAX_CONTENT_LEN
*  Maximum length of one log content fragment.
*
*/
#ifndef LOG_ENTRY_MAX_CONTENT_LEN
  #define LOG_ENTRY_MAX_CONTENT_LEN   1024
#endif

/*********************************************************************
*
*       Types
*
**********************************************************************
*/

/*********************************************************************
*
*       LogSource_t
*
*  Description
*    Log source identifier for swimlane display.
*
*  Values
*    LOG_SOURCE_LINUX  Log from Linux A53 core
*    LOG_SOURCE_RTOS   Log from RTOS R5 core
*/
typedef enum {
    LOG_SOURCE_LINUX = 0,
    LOG_SOURCE_RTOS  = 1,
    LOG_SOURCE_MAX
} LogSource_t;

/*********************************************************************
*
*       LogEntry_t
*
*  Description
*    Single log entry with timestamp, source, and content.
*
*  Fields
*    timestamp_us  Unified microsecond timestamp
*    source        Log source (LINUX or RTOS)
*    sequence      Source-local sequence used to order equal timestamps
*    content       Log content string (dynamically allocated)
*    content_len   Length of content string
*    fragment_continuation  This entry continues a previous fragment
*    fragment_continues     Another fragment follows this entry
*    valid         Entry validity flag
*/
typedef struct {
    uint64_t      timestamp_us;
    LogSource_t   source;
    uint64_t      sequence;
    char         *content;
    size_t        content_len;
    bool          fragment_continuation;
    bool          fragment_continues;
    bool          valid;
} LogEntry_t;

/*********************************************************************
*
*       API functions
*
**********************************************************************
*/

LogEntry_t* LogEntry_Create         (uint64_t timestamp_us, LogSource_t source,
                                     const char *content, size_t content_len);
LogEntry_t* LogEntry_CreateEx       (uint64_t timestamp_us, LogSource_t source,
                                     uint64_t sequence,
                                     bool fragment_continuation,
                                     bool fragment_continues,
                                     const char *content, size_t content_len);
LogEntry_t* LogEntry_Clone          (const LogEntry_t *entry);
void        LogEntry_Destroy        (LogEntry_t *entry);
uint64_t    LogEntry_GetTimestamp   (const LogEntry_t *entry);
LogSource_t LogEntry_GetSource      (const LogEntry_t *entry);
uint64_t    LogEntry_GetSequence    (const LogEntry_t *entry);
const char* LogEntry_GetContent     (const LogEntry_t *entry);
size_t      LogEntry_GetContentLen  (const LogEntry_t *entry);
bool        LogEntry_IsFragmentContinuation(const LogEntry_t *entry);
bool        LogEntry_FragmentContinues     (const LogEntry_t *entry);
bool        LogEntry_IsValid        (const LogEntry_t *entry);
int         LogEntry_Compare        (const LogEntry_t *a, const LogEntry_t *b);

#if defined(__cplusplus)          // Allow usage of this module from C++ files (disable name mangling)
}                /* Make sure we have C-declarations in C++ programs */
#endif

#endif // end of include guard: TRACEHUB_LOGENTRY_H Avoid multiple inclusion

/*************************** End of file ****************************/
