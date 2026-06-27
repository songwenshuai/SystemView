/*********************************************************************
*                                                                    *
*                Copyright (C) 2023 xrTest Inc.                      *
*                      All rights reserved                           *
*                                                                    *
**********************************************************************
----------------------------------------------------------------------
File    : test_logline_parser.c
Purpose : Unit checks for core log timestamp parsing
Author  : songwenshuai <songwenshuai@gmail.com>
---------------------------END-OF-HEADER------------------------------
*/

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "LogLineParser.h"

/*********************************************************************
*
*       Defines
*
**********************************************************************
*/

#define TEST_ASSERT(expr)                                                   \
    do {                                                                    \
        if (!(expr)) {                                                       \
            fprintf(stderr, "%s:%d: assertion failed: %s\n",                \
                    __FILE__, __LINE__, #expr);                             \
            return -1;                                                      \
        }                                                                   \
    } while (0)

/*********************************************************************
*
*       Static functions
*
**********************************************************************
*/

static int _ExpectTimestamp(const char *line, uint64_t expected_timestamp,
                            const char *expected_content) {
    uint64_t    timestamp;
    const char *content;

    timestamp = 0u;
    content   = NULL;
    TEST_ASSERT(LogLineParser_ParseTimestampPrefix(line, &timestamp, &content) ==
                LOG_LINE_TIMESTAMP_PARSE_PRESENT);
    TEST_ASSERT(timestamp == expected_timestamp);
    TEST_ASSERT(content != NULL);
    TEST_ASSERT(strcmp(content, expected_content) == 0);
    return 0;
}

static int _ExpectNoTimestamp(const char *line) {
    uint64_t    timestamp;
    const char *content;

    timestamp = 0u;
    content   = NULL;
    TEST_ASSERT(LogLineParser_ParseTimestampPrefix(line, &timestamp, &content) ==
                LOG_LINE_TIMESTAMP_PARSE_NONE);
    TEST_ASSERT(content == NULL);
    return 0;
}

/*********************************************************************
*
*       main()
*/
int main(void) {
    if (_ExpectTimestamp("[123] message", 123u, "message") != 0) {
        return 1;
    }
    if (_ExpectTimestamp("  [12:00:00.100] clock", 43200100000ULL, "clock") != 0) {
        return 1;
    }
    if (_ExpectTimestamp("[12:00:00] whole-second", 43200000000ULL, "whole-second") != 0) {
        return 1;
    }

    if (_ExpectNoTimestamp("123 message") != 0) {
        return 1;
    }
    if (_ExpectNoTimestamp("12:00:00 message") != 0) {
        return 1;
    }
    if (_ExpectNoTimestamp("[12:00:00.1] clock") != 0) {
        return 1;
    }
    if (_ExpectNoTimestamp("[12:00:00.1000] clock") != 0) {
        return 1;
    }

    return 0;
}

/*************************** End of file ****************************/
