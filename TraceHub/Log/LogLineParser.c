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
File    : LogLineParser.c
Purpose : Log line parsing helpers for timestamped core logs
---------------------------END-OF-HEADER------------------------------
*/

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "LogLineParser.h"

/*********************************************************************
*
*       Defines
*
**********************************************************************
*/

#define LOG_TIMESTAMP_US_PER_MILLISECOND 1000ULL
#define LOG_TIMESTAMP_US_PER_SECOND      1000000ULL
#define LOG_TIMESTAMP_SECONDS_PER_MINUTE 60ULL
#define LOG_TIMESTAMP_SECONDS_PER_HOUR   3600ULL
#define LOG_TIMESTAMP_MILLISECOND_DIGITS 3u

/*********************************************************************
*
*       Static functions
*
**********************************************************************
*/

/*********************************************************************
*
*       _Utf8ExpectedLength()
*
*  Function description
*    Return the expected byte length for a UTF-8 leading byte.
*/
static unsigned _Utf8ExpectedLength(unsigned char ch) {
    if ((ch & 0x80u) == 0u) {
        return 1u;
    }
    if ((ch & 0xE0u) == 0xC0u) {
        return 2u;
    }
    if ((ch & 0xF0u) == 0xE0u) {
        return 3u;
    }
    if ((ch & 0xF8u) == 0xF0u) {
        return 4u;
    }
    return 0u;
}

/*********************************************************************
*
*       _IsUtf8Continuation()
*
*  Function description
*    Check whether a byte is a UTF-8 continuation byte.
*/
static bool _IsUtf8Continuation(unsigned char ch) {
    return ((ch & 0xC0u) == 0x80u);
}

/*********************************************************************
*
*       _IsHorizontalWhitespace()
*
*  Function description
*    Check for spaces accepted between a timestamp and log content.
*/
static bool _IsHorizontalWhitespace(char c) {
    return (c == ' ' || c == '\t');
}

/*********************************************************************
*
*       _ParseDecimalValue()
*
*  Function description
*    Parse an unsigned decimal integer with overflow checking.
*/
static int _ParseDecimalValue(const char **cursor, uint64_t *value) {
    const char *p;
    uint64_t    parsed;
    unsigned    digit;

    if (cursor == NULL || *cursor == NULL || value == NULL) {
        return -1;
    }

    p = *cursor;
    if (*p < '0' || *p > '9') {
        return -1;
    }

    parsed = 0u;
    while (*p >= '0' && *p <= '9') {
        digit = (unsigned)(*p - '0');
        if (parsed > ((UINT64_MAX - (uint64_t)digit) / 10u)) {
            return -1;
        }
        parsed = (parsed * 10u) + (uint64_t)digit;
        p++;
    }

    *cursor = p;
    *value = parsed;
    return 0;
}

/*********************************************************************
*
*       _ParseMillisecondValue()
*
*  Function description
*    Parse the fixed-width mmm field in HH:MM:SS.mmm.
*/
static int _ParseMillisecondValue(const char **cursor, uint64_t *value) {
    const char *p;
    uint64_t    parsed;
    unsigned    i;

    if (cursor == NULL || *cursor == NULL || value == NULL) {
        return -1;
    }

    p      = *cursor;
    parsed = 0u;
    for (i = 0u; i < LOG_TIMESTAMP_MILLISECOND_DIGITS; i++) {
        if (*p < '0' || *p > '9') {
            return -1;
        }
        parsed = (parsed * 10u) + (uint64_t)(*p - '0');
        p++;
    }
    if (*p >= '0' && *p <= '9') {
        return -1;
    }

    *cursor = p;
    *value  = parsed;
    return 0;
}

/*********************************************************************
*
*       _BuildClockTimestamp()
*
*  Function description
*    Convert HH:MM:SS.mmm fields to microseconds.
*/
static int _BuildClockTimestamp(uint64_t hours, uint64_t minutes,
                                uint64_t seconds, uint64_t milliseconds,
                                uint64_t *timestamp_us) {
    uint64_t total_seconds;
    uint64_t total_us;

    if (timestamp_us == NULL) {
        return -1;
    }
    if (minutes >= LOG_TIMESTAMP_SECONDS_PER_MINUTE ||
        seconds >= LOG_TIMESTAMP_SECONDS_PER_MINUTE ||
        milliseconds >= LOG_TIMESTAMP_US_PER_MILLISECOND) {
        return -1;
    }
    if (hours > (UINT64_MAX / LOG_TIMESTAMP_SECONDS_PER_HOUR)) {
        return -1;
    }

    total_seconds = hours * LOG_TIMESTAMP_SECONDS_PER_HOUR;
    if (total_seconds > (UINT64_MAX - (minutes * LOG_TIMESTAMP_SECONDS_PER_MINUTE))) {
        return -1;
    }
    total_seconds += minutes * LOG_TIMESTAMP_SECONDS_PER_MINUTE;
    if (total_seconds > (UINT64_MAX - seconds)) {
        return -1;
    }
    total_seconds += seconds;
    if (total_seconds > (UINT64_MAX / LOG_TIMESTAMP_US_PER_SECOND)) {
        return -1;
    }

    total_us = total_seconds * LOG_TIMESTAMP_US_PER_SECOND;
    if (total_us > (UINT64_MAX - (milliseconds * LOG_TIMESTAMP_US_PER_MILLISECOND))) {
        return -1;
    }
    total_us += milliseconds * LOG_TIMESTAMP_US_PER_MILLISECOND;

    *timestamp_us = total_us;
    return 0;
}

/*********************************************************************
*
*       _ParseClockTimestamp()
*
*  Function description
*    Parse HH:MM:SS or HH:MM:SS.mmm timestamp fields.
*/
static int _ParseClockTimestamp(const char **cursor, uint64_t *timestamp_us) {
    const char *p;
    uint64_t    hours;
    uint64_t    minutes;
    uint64_t    seconds;
    uint64_t    milliseconds;

    if (cursor == NULL || *cursor == NULL || timestamp_us == NULL) {
        return -1;
    }

    p = *cursor;
    if (_ParseDecimalValue(&p, &hours) != 0) {
        return -1;
    }
    if (*p != ':') {
        return -1;
    }
    p++;
    if (_ParseDecimalValue(&p, &minutes) != 0) {
        return -1;
    }
    if (*p != ':') {
        return -1;
    }
    p++;
    if (_ParseDecimalValue(&p, &seconds) != 0) {
        return -1;
    }

    milliseconds = 0u;
    if (*p == '.') {
        p++;
        if (_ParseMillisecondValue(&p, &milliseconds) != 0) {
            return -1;
        }
    }
    if (_BuildClockTimestamp(hours, minutes, seconds, milliseconds, timestamp_us) != 0) {
        return -1;
    }

    *cursor = p;
    return 0;
}

/*********************************************************************
*
*       _ParseTimestampValue()
*
*  Function description
*    Parse one accepted timestamp value.
*/
static int _ParseTimestampValue(const char **cursor, uint64_t *timestamp_us) {
    const char *p;

    if (cursor == NULL || *cursor == NULL || timestamp_us == NULL) {
        return -1;
    }

    p = *cursor;
    if (_ParseClockTimestamp(&p, timestamp_us) == 0) {
        *cursor = p;
        return 0;
    }

    p = *cursor;
    if (_ParseDecimalValue(&p, timestamp_us) != 0) {
        return -1;
    }
    *cursor = p;
    return 0;
}

/*********************************************************************
*
*       Public functions
*
**********************************************************************
*/

/*********************************************************************
*
*       LogLineParser_TrimTrailingWhitespace()
*/
size_t LogLineParser_TrimTrailingWhitespace(char *str) {
    size_t len;

    if (str == NULL) {
        return 0;
    }

    len = strlen(str);
    while (len > 0 && (str[len - 1] == ' ' || str[len - 1] == '\t' ||
                       str[len - 1] == '\n' || str[len - 1] == '\r')) {
        str[len - 1] = '\0';
        len--;
    }
    return len;
}

/*********************************************************************
*
*       LogLineParser_AdjustUtf8Boundary()
*/
size_t LogLineParser_AdjustUtf8Boundary(const char *str, size_t len) {
    size_t   lead_index;
    unsigned expected_len;
    unsigned actual_len;

    if (str == NULL || len == 0u) {
        return 0u;
    }

    lead_index = len;
    while (lead_index > 0u &&
           _IsUtf8Continuation((unsigned char)str[lead_index - 1u])) {
        lead_index--;
    }

    if (lead_index == len) {
        expected_len = _Utf8ExpectedLength((unsigned char)str[len - 1u]);
        if (expected_len > 1u) {
            return len - 1u;
        }
        return len;
    }

    if (lead_index == 0u) {
        return 0u;
    }

    lead_index--;
    expected_len = _Utf8ExpectedLength((unsigned char)str[lead_index]);
    if (expected_len == 0u) {
        return len;
    }

    actual_len = (unsigned)(len - lead_index);
    if (actual_len < expected_len) {
        return lead_index;
    }
    return len;
}

/*********************************************************************
*
*       LogLineParser_SkipHorizontalWhitespace()
*/
const char *LogLineParser_SkipHorizontalWhitespace(const char *p) {
    if (p == NULL) {
        return NULL;
    }

    while (_IsHorizontalWhitespace(*p)) {
        p++;
    }
    return p;
}

/*********************************************************************
*
*       LogLineParser_ParseTimestampPrefix()
*/
LogLineTimestampParseResult_t LogLineParser_ParseTimestampPrefix(const char *line,
                                                                 uint64_t *timestamp_us,
                                                                 const char **content) {
    const char *p;
    const char *value_start;

    if (line == NULL || timestamp_us == NULL || content == NULL) {
        return LOG_LINE_TIMESTAMP_PARSE_NONE;
    }

    p = LogLineParser_SkipHorizontalWhitespace(line);
    if (*p != '[') {
        return LOG_LINE_TIMESTAMP_PARSE_NONE;
    }

    p++;
    value_start = p;
    if (_ParseTimestampValue(&p, timestamp_us) != 0) {
        return LOG_LINE_TIMESTAMP_PARSE_NONE;
    }
    if (*p != ']') {
        return LOG_LINE_TIMESTAMP_PARSE_NONE;
    }
    p++;

    if (p == value_start) {
        return LOG_LINE_TIMESTAMP_PARSE_NONE;
    }
    if (*p == '\0') {
        return LOG_LINE_TIMESTAMP_PARSE_EMPTY_CONTENT;
    }
    if (!_IsHorizontalWhitespace(*p)) {
        return LOG_LINE_TIMESTAMP_PARSE_NONE;
    }

    p = LogLineParser_SkipHorizontalWhitespace(p);
    if (*p == '\0') {
        return LOG_LINE_TIMESTAMP_PARSE_EMPTY_CONTENT;
    }

    *content = p;
    return LOG_LINE_TIMESTAMP_PARSE_PRESENT;
}

/*************************** End of file ****************************/
