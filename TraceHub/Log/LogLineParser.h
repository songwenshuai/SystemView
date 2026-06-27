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
File    : LogLineParser.h
Purpose : Log line parsing helpers for timestamped core logs
---------------------------END-OF-HEADER------------------------------
*/

#ifndef TRACEHUB_LOGLINEPARSER_H
#define TRACEHUB_LOGLINEPARSER_H

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#if defined(__cplusplus)
extern "C" {
#endif

/*********************************************************************
*
*       Types
*
**********************************************************************
*/

typedef enum {
    LOG_LINE_TIMESTAMP_PARSE_NONE = 0,
    LOG_LINE_TIMESTAMP_PARSE_PRESENT,
    LOG_LINE_TIMESTAMP_PARSE_EMPTY_CONTENT
} LogLineTimestampParseResult_t;

/*********************************************************************
*
*       API functions
*
**********************************************************************
*/

size_t                         LogLineParser_TrimTrailingWhitespace  (char *str);
size_t                         LogLineParser_AdjustUtf8Boundary      (const char *str, size_t len);
const char *                   LogLineParser_SkipHorizontalWhitespace(const char *p);
LogLineTimestampParseResult_t  LogLineParser_ParseTimestampPrefix    (const char *line, uint64_t *timestamp_us, const char **content);

#if defined(__cplusplus)
}
#endif

#endif /* TRACEHUB_LOGLINEPARSER_H */

/*************************** End of file ****************************/
