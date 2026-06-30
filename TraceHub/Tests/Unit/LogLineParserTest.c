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
File    : LogLineParserTest.c
Purpose : Unit checks for core log timestamp parsing
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

static int _ExpectEmptyContent(const char *line) {
    uint64_t    timestamp;
    const char *content;

    timestamp = 0u;
    content   = NULL;
    TEST_ASSERT(LogLineParser_ParseTimestampPrefix(line, &timestamp, &content) ==
                LOG_LINE_TIMESTAMP_PARSE_EMPTY_CONTENT);
    TEST_ASSERT(content == NULL);
    return 0;
}

static int _TestWhitespaceHelpers(void) {
    char        line[] = "message \t\r\n";
    const char *p;

    TEST_ASSERT(LogLineParser_TrimTrailingWhitespace(line) == strlen("message"));
    TEST_ASSERT(strcmp(line, "message") == 0);
    TEST_ASSERT(LogLineParser_TrimTrailingWhitespace(NULL) == 0u);

    p = LogLineParser_SkipHorizontalWhitespace(" \tcontent");
    TEST_ASSERT(p != NULL);
    TEST_ASSERT(strcmp(p, "content") == 0);
    TEST_ASSERT(LogLineParser_SkipHorizontalWhitespace(NULL) == NULL);
    return 0;
}

static int _TestUtf8BoundaryAdjustment(void) {
    static const char text[] = "A\xE2\x82\xAC""B";
    static const char continuation[] = "\x82";

    TEST_ASSERT(LogLineParser_AdjustUtf8Boundary(NULL, 1u) == 0u);
    TEST_ASSERT(LogLineParser_AdjustUtf8Boundary(text, 0u) == 0u);
    TEST_ASSERT(LogLineParser_AdjustUtf8Boundary(text, 1u) == 1u);
    TEST_ASSERT(LogLineParser_AdjustUtf8Boundary(text, 2u) == 1u);
    TEST_ASSERT(LogLineParser_AdjustUtf8Boundary(text, 3u) == 1u);
    TEST_ASSERT(LogLineParser_AdjustUtf8Boundary(text, 4u) == 4u);
    TEST_ASSERT(LogLineParser_AdjustUtf8Boundary(text, 5u) == 5u);
    TEST_ASSERT(LogLineParser_AdjustUtf8Boundary(continuation, 1u) == 0u);
    return 0;
}

/*********************************************************************
*
*       main()
*
*  Function description
*    Run LogLineParser unit tests.
*
*  Return value
*    0  All tests passed.
*    1  A test failed.
*/
int main(void) {
    if (_TestWhitespaceHelpers() != 0) {
        return 1;
    }
    if (_TestUtf8BoundaryAdjustment() != 0) {
        return 1;
    }

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
    if (_ExpectNoTimestamp("[12:60:00] clock") != 0) {
        return 1;
    }
    if (_ExpectNoTimestamp("[12:00:60] clock") != 0) {
        return 1;
    }
    if (_ExpectNoTimestamp("[123]message") != 0) {
        return 1;
    }
    if (_ExpectEmptyContent("[123]") != 0) {
        return 1;
    }
    if (_ExpectEmptyContent("[123] \t") != 0) {
        return 1;
    }
    if (LogLineParser_ParseTimestampPrefix(NULL, NULL, NULL) !=
        LOG_LINE_TIMESTAMP_PARSE_NONE) {
        return 1;
    }

    return 0;
}

/*************************** End of file ****************************/
