/*********************************************************************
*                                                                    *
*                Copyright (C) 2023 xrTest Inc.                      *
*                      All rights reserved                           *
*                                                                    *
**********************************************************************
----------------------------------------------------------------------
File    : LogLineParser.h
Purpose : Log line parsing helpers for timestamped core logs
Author  : songwenshuai <songwenshuai@gmail.com>
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

size_t LogLineParser_TrimTrailingWhitespace(char *str);
size_t LogLineParser_AdjustUtf8Boundary(const char *str, size_t len);
const char *LogLineParser_SkipHorizontalWhitespace(const char *p);
LogLineTimestampParseResult_t LogLineParser_ParseTimestampPrefix(const char *line,
                                                                 uint64_t *timestamp_us,
                                                                 const char **content);

#if defined(__cplusplus)
}
#endif

#endif /* TRACEHUB_LOGLINEPARSER_H */

/*************************** End of file ****************************/
