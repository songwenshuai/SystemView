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
File    : SwimLaneRendererTest.c
Purpose : Unit checks for swimlane layout resolution
---------------------------END-OF-HEADER------------------------------
*/

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include <locale.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "SwimLaneRenderer_internal.h"
#include "SYS.h"

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
*       Runtime dependency stubs
*
**********************************************************************
*/

int SYS_MutexInit(SYS_Mutex *mutex) {
    (void)mutex;
    return 0;
}

int SYS_MutexLock(SYS_Mutex *mutex) {
    (void)mutex;
    return 0;
}

int SYS_MutexUnlock(SYS_Mutex *mutex) {
    (void)mutex;
    return 0;
}

int SYS_MutexDestroy(SYS_Mutex *mutex) {
    (void)mutex;
    return 0;
}

/*********************************************************************
*
*       Static functions
*
**********************************************************************
*/

static int _TestNarrowWidthClampsToMinimumLayout(void) {
    unsigned linux_width;
    unsigned rtos_width;

    TEST_ASSERT(SwimLaneLayout_ResolveColumnWidths(1u,
                                                   SWIMLANE_DEFAULT_TIMESTAMP_WIDTH,
                                                   true,
                                                   &linux_width,
                                                   &rtos_width) == 0);
    TEST_ASSERT(linux_width >= SWIMLANE_MIN_SOURCE_WIDTH);
    TEST_ASSERT(rtos_width >= SWIMLANE_MIN_SOURCE_WIDTH);
    return 0;
}

static int _TestExplicitNarrowWidthFails(void) {
    unsigned linux_width;
    unsigned rtos_width;

    TEST_ASSERT(SwimLaneLayout_ResolveColumnWidths(1u,
                                                   SWIMLANE_DEFAULT_TIMESTAMP_WIDTH,
                                                   false,
                                                   &linux_width,
                                                   &rtos_width) != 0);
    return 0;
}

static int _TestNormalWidthUsesAvailableColumns(void) {
    unsigned linux_width;
    unsigned rtos_width;
    unsigned source_width;

    TEST_ASSERT(SwimLaneLayout_ResolveColumnWidths(120u,
                                                   SWIMLANE_DEFAULT_TIMESTAMP_WIDTH,
                                                   false,
                                                   &linux_width,
                                                   &rtos_width) == 0);

    source_width = 120u - SWIMLANE_DEFAULT_TIMESTAMP_WIDTH - SWIMLANE_SEPARATOR_WIDTH;
    TEST_ASSERT(linux_width + rtos_width == source_width);
    TEST_ASSERT(linux_width == source_width / 2u);
    TEST_ASSERT(rtos_width == source_width - linux_width);
    return 0;
}

static int _TestExplicitOversizedWidthFails(void) {
    SwimLane_Config_t config;
    SwimLane_Layout_t layout;

    memset(&config, 0, sizeof(config));
    memset(&layout, 0, sizeof(layout));
    config.total_width = UINT_MAX;
    TEST_ASSERT(SwimLaneLayout_Resolve(&config, &layout) != 0);
    return 0;
}

static int _TestCellAndLayoutBoundaryContracts(void) {
    SwimLane_Config_t config;
    SwimLane_Layout_t layout;
    FILE             *file;
    char              buffer[128];
    size_t            len;
    unsigned          linux_width;
    unsigned          rtos_width;
    unsigned          printed;

    memset(&config, 0, sizeof(config));
    memset(&layout, 0, sizeof(layout));
    TEST_ASSERT(SwimLaneLayout_ResolveColumnWidths(80u,
                                                   SWIMLANE_DEFAULT_TIMESTAMP_WIDTH,
                                                   false,
                                                   NULL,
                                                   &rtos_width) == -1);
    TEST_ASSERT(SwimLaneLayout_ResolveColumnWidths(80u,
                                                   SWIMLANE_DEFAULT_TIMESTAMP_WIDTH,
                                                   false,
                                                   &linux_width,
                                                   NULL) == -1);
    TEST_ASSERT(SwimLaneLayout_ResolveColumnWidths(80u,
                                                   UINT_MAX,
                                                   true,
                                                   &linux_width,
                                                   &rtos_width) == -1);
    TEST_ASSERT(SwimLaneLayout_Resolve(NULL, &layout) == -1);
    TEST_ASSERT(SwimLaneLayout_Resolve(&config, NULL) == -1);
    TEST_ASSERT(SwimLaneLayout_Resolve(&config, &layout) == 0);
    TEST_ASSERT(layout.timestamp_width == SWIMLANE_DEFAULT_TIMESTAMP_WIDTH);
    TEST_ASSERT(layout.linux_width >= SWIMLANE_MIN_SOURCE_WIDTH);
    TEST_ASSERT(layout.rtos_width >= SWIMLANE_MIN_SOURCE_WIDTH);
    TEST_ASSERT(strcmp(layout.linux_label, SWIMLANE_DEFAULT_LINUX_LABEL) == 0);
    TEST_ASSERT(strcmp(layout.rtos_label, SWIMLANE_DEFAULT_RTOS_LABEL) == 0);

    memset(&config, 0, sizeof(config));
    memset(&layout, 0, sizeof(layout));
    config.total_width = 1u;
    TEST_ASSERT(SwimLaneLayout_Resolve(&config, &layout) == -1);

    file = tmpfile();
    TEST_ASSERT(file != NULL);
    TEST_ASSERT(SwimLaneCell_Print(file,
                                   "abcdef",
                                   (unsigned)INT_MAX + 1u,
                                   NULL,
                                   0u,
                                   false) == 0u);
    printed = SwimLaneCell_Print(file, NULL, 4u, NULL, 0u, false);
    TEST_ASSERT(printed == 0u);
    printed = SwimLaneCell_Print(file, "abc\r\n", 4u, NULL, 8u, false);
    TEST_ASSERT(printed == 0u);
    printed = SwimLaneCell_Print(file, "abcdef", 3u, ANSI_COLOR_LINUX, 2u, true);
    TEST_ASSERT(printed == 3u);
    SwimLaneCell_PrintTimestamp(file, 3723004005ULL, 12u, true, false);
    SwimLaneCell_PrintTimestamp(file, 0u, 12u, false, false);

    TEST_ASSERT(fflush(file) == 0);
    rewind(file);
    len = fread(buffer, 1u, sizeof(buffer) - 1u, file);
    TEST_ASSERT(!ferror(file));
    buffer[len] = '\0';
    TEST_ASSERT(strstr(buffer, "cde") != NULL);
    TEST_ASSERT(strstr(buffer, "01:02:03.004") != NULL);
    TEST_ASSERT(strstr(buffer, ANSI_COLOR_LINUX) != NULL);
    TEST_ASSERT(fclose(file) == 0);
    return 0;
}

static int _TestPublicRenderLifecycle(void) {
    SwimLane_Config_t        config;
    const SwimLane_State_t  *state;
    LogEntry_t              *entry;
    FILE                    *file;
    char                     buffer[512];
    size_t                   len;

    file = tmpfile();
    TEST_ASSERT(file != NULL);

    memset(&config, 0, sizeof(config));
    config.total_width    = 80u;
    config.show_header    = true;
    config.show_separator = true;
    config.color_enabled  = false;
    config.output_stream  = file;

    TEST_ASSERT(SwimLane_RenderHeader() == -1);
    TEST_ASSERT(SwimLane_RenderSeparator() == -1);
    TEST_ASSERT(SwimLane_RenderEntry(NULL) == -1);
    TEST_ASSERT(SwimLane_GetState() == NULL);
    TEST_ASSERT(SwimLane_Init(NULL) == -1);
    TEST_ASSERT(SwimLane_Init(&config) == 0);
    TEST_ASSERT(SwimLane_Init(&config) == -1);

    state = SwimLane_GetState();
    TEST_ASSERT(state != NULL);
    TEST_ASSERT(state->initialized);
    TEST_ASSERT(!state->header_shown);
    TEST_ASSERT(SwimLane_GetRowCount() == 0u);
    TEST_ASSERT(SwimLane_RenderEntry(NULL) == -2);
    TEST_ASSERT(SwimLane_RenderHeader() == 0);
    TEST_ASSERT(state->header_shown);
    TEST_ASSERT(SwimLane_RenderHeader() == 0);
    TEST_ASSERT(SwimLane_RenderSeparator() == 0);

    entry = LogEntry_Create(61000000u, LOG_SOURCE_LINUX, "left side", strlen("left side"));
    TEST_ASSERT(entry != NULL);
    TEST_ASSERT(SwimLane_RenderEntry(entry) == 0);
    TEST_ASSERT(SwimLane_GetRowCount() == 1u);
    LogEntry_Destroy(entry);

    TEST_ASSERT(fflush(file) == 0);
    rewind(file);
    len = fread(buffer, 1u, sizeof(buffer) - 1u, file);
    TEST_ASSERT(!ferror(file));
    buffer[len] = '\0';
    TEST_ASSERT(strstr(buffer, "LINUX") != NULL);
    TEST_ASSERT(strstr(buffer, "left side") != NULL);

    SwimLane_Cleanup();
    TEST_ASSERT(SwimLane_GetState() == NULL);
    TEST_ASSERT(SwimLane_GetRowCount() == 0u);
    TEST_ASSERT(fclose(file) == 0);
    return 0;
}

static int _TestRenderEntryAutoHeaderAndColorBranches(void) {
    SwimLane_Config_t config;
    LogEntry_t       *entry;
    FILE             *file;
    char              buffer[1024];
    size_t            len;

    file = tmpfile();
    TEST_ASSERT(file != NULL);

    memset(&config, 0, sizeof(config));
    config.total_width    = 80u;
    config.show_header    = true;
    config.show_separator = true;
    config.color_enabled  = false;
    config.output_stream  = file;

    TEST_ASSERT(SwimLane_Init(&config) == 0);
    _swimlane_state.config.color_enabled = true;
    TEST_ASSERT(SwimLane_RenderHeader() == 0);
    TEST_ASSERT(SwimLane_RenderSeparator() == 0);
    TEST_ASSERT(fflush(file) == 0);
    rewind(file);
    len = fread(buffer, 1u, sizeof(buffer) - 1u, file);
    TEST_ASSERT(!ferror(file));
    buffer[len] = '\0';
    TEST_ASSERT(strstr(buffer, ANSI_COLOR_HEADER) != NULL);
    TEST_ASSERT(strstr(buffer, ANSI_COLOR_SEPARATOR) != NULL);
    TEST_ASSERT(strstr(buffer, ANSI_COLOR_RESET) != NULL);
    SwimLane_Cleanup();
    TEST_ASSERT(fclose(file) == 0);

    file = tmpfile();
    TEST_ASSERT(file != NULL);
    memset(&config, 0, sizeof(config));
    config.total_width    = 80u;
    config.show_header    = true;
    config.show_separator = false;
    config.color_enabled  = false;
    config.output_stream  = file;

    TEST_ASSERT(SwimLane_Init(&config) == 0);
    entry = LogEntry_Create(3723004005ULL,
                            LOG_SOURCE_RTOS,
                            "rtos side\r\n",
                            strlen("rtos side\r\n"));
    TEST_ASSERT(entry != NULL);
    TEST_ASSERT(SwimLane_RenderEntry(entry) == 0);
    LogEntry_Destroy(entry);

    entry = LogEntry_CreateEx(3723004006ULL,
                              LOG_SOURCE_RTOS,
                              1u,
                              true,
                              false,
                              "continued",
                              strlen("continued"));
    TEST_ASSERT(entry != NULL);
    TEST_ASSERT(SwimLane_RenderEntry(entry) == 0);
    LogEntry_Destroy(entry);
    TEST_ASSERT(SwimLane_GetRowCount() == 2u);

    TEST_ASSERT(fflush(file) == 0);
    rewind(file);
    len = fread(buffer, 1u, sizeof(buffer) - 1u, file);
    TEST_ASSERT(!ferror(file));
    buffer[len] = '\0';
    TEST_ASSERT(strstr(buffer, "RTOS") != NULL);
    TEST_ASSERT(strstr(buffer, "rtos side") != NULL);
    TEST_ASSERT(strstr(buffer, "continued") != NULL);
    TEST_ASSERT(strstr(buffer, "01:02:03.004") != NULL);

    SwimLane_Cleanup();
    TEST_ASSERT(fclose(file) == 0);
    return 0;
}

static int _TestTextAnsiAndUtf8Helpers(void) {
    FILE         *file;
    char          buffer[128];
    size_t        len;
    unsigned      cols;
    unsigned      width_bytes;
    const char   *colored = "A\033[31mB\033[0mC";
    const char   *osc = "\033]0;title\007";
    const char   *osc_st = "\033]0;title\033\\";
    const char   *csi = "\033[31m";
    const char    esc_only[] = { '\033', '\0' };
    const char    unknown_escape[] = { '\033', '?', '\0' };
    const char    truncated_escape[] = { '\033', '[', '3', '1', 'm', 'X' };
    const char    invalid_utf8[] = { (char)0xff, 'Z', '\0' };
    const char    incomplete_utf8[] = { (char)0xe2, (char)0x82, '\0' };
    const char    wide_text[] = { (char)0xe4, (char)0xb8, (char)0xad };
    const char    nul_text[] = { 'A', '\0', 'B' };
    const char    control_text[] = { '\001', 'A' };

    (void)setlocale(LC_CTYPE, "");

    TEST_ASSERT(SwimLaneText_AnsiEscapeLength(NULL) == 0u);
    TEST_ASSERT(SwimLaneText_AnsiEscapeLength("plain") == 0u);
    TEST_ASSERT(SwimLaneText_AnsiEscapeLength(esc_only) == 0u);
    TEST_ASSERT(SwimLaneText_AnsiEscapeLength(csi) == strlen(csi));
    TEST_ASSERT(SwimLaneText_AnsiEscapeLength(osc) == strlen(osc));
    TEST_ASSERT(SwimLaneText_AnsiEscapeLength(osc_st) == strlen(osc_st));
    TEST_ASSERT(SwimLaneText_AnsiEscapeLength("\033M") == 2u);
    TEST_ASSERT(SwimLaneText_AnsiEscapeLength(unknown_escape) == 0u);
    TEST_ASSERT(SwimLaneText_AnsiEscapeLength("\033[") == 0u);
    TEST_ASSERT(SwimLaneText_AnsiEscapeLength("\033]unterminated") == 0u);

    SwimLaneText_WriteBytes(NULL, colored, (unsigned)strlen(colored), false);
    SwimLaneText_WriteBytes(stdout, NULL, 0u, false);

    file = tmpfile();
    TEST_ASSERT(file != NULL);
    SwimLaneText_WriteBytes(file, colored, (unsigned)strlen(colored), false);
    TEST_ASSERT(fflush(file) == 0);
    rewind(file);
    len = fread(buffer, 1u, sizeof(buffer) - 1u, file);
    TEST_ASSERT(!ferror(file));
    buffer[len] = '\0';
    TEST_ASSERT(strcmp(buffer, "ABC") == 0);
    TEST_ASSERT(fclose(file) == 0);

    file = tmpfile();
    TEST_ASSERT(file != NULL);
    SwimLaneText_WriteBytes(file, colored, (unsigned)strlen(colored), true);
    TEST_ASSERT(fflush(file) == 0);
    rewind(file);
    len = fread(buffer, 1u, sizeof(buffer) - 1u, file);
    TEST_ASSERT(!ferror(file));
    buffer[len] = '\0';
    TEST_ASSERT(strcmp(buffer, colored) == 0);
    TEST_ASSERT(fclose(file) == 0);

    file = tmpfile();
    TEST_ASSERT(file != NULL);
    SwimLaneText_WriteBytes(file, truncated_escape, 2u, false);
    TEST_ASSERT(fflush(file) == 0);
    rewind(file);
    len = fread(buffer, 1u, sizeof(buffer) - 1u, file);
    TEST_ASSERT(!ferror(file));
    buffer[len] = '\0';
    TEST_ASSERT(len == 2u);
    TEST_ASSERT(buffer[0] == '\033');
    TEST_ASSERT(buffer[1] == '[');
    TEST_ASSERT(fclose(file) == 0);

    TEST_ASSERT(SwimLaneText_Utf8BytesForWidth(NULL, 1u, 1u, &cols) == 0u);
    TEST_ASSERT(cols == 0u);
    TEST_ASSERT(SwimLaneText_Utf8BytesForWidth("x", 0u, 1u, &cols) == 0u);
    TEST_ASSERT(cols == 0u);
    TEST_ASSERT(SwimLaneText_Utf8BytesForWidth("x", 1u, 0u, &cols) == 0u);
    TEST_ASSERT(cols == 0u);
    TEST_ASSERT(SwimLaneText_Utf8BytesForWidth("abcdef", 6u, 3u, &cols) == 3u);
    TEST_ASSERT(cols == 3u);
    TEST_ASSERT(SwimLaneText_Utf8BytesForWidth("abc", 3u, 2u, NULL) == 2u);
    TEST_ASSERT(SwimLaneText_Utf8BytesForWidth("A\033[31mB", 7u, 2u, &cols) == 7u);
    TEST_ASSERT(cols == 2u);
    TEST_ASSERT(SwimLaneText_Utf8BytesForWidth("A\033[31m", 6u, 1u, &cols) == 6u);
    TEST_ASSERT(cols == 1u);
    TEST_ASSERT(SwimLaneText_Utf8BytesForWidth("A\033[31m", 3u, 1u, &cols) == 1u);
    TEST_ASSERT(cols == 1u);
    TEST_ASSERT(SwimLaneText_Utf8BytesForWidth(nul_text, sizeof(nul_text), 3u, &cols) == 1u);
    TEST_ASSERT(cols == 1u);
    TEST_ASSERT(SwimLaneText_Utf8BytesForWidth(control_text, sizeof(control_text), 1u, &cols) == 1u);
    TEST_ASSERT(cols == 1u);
    TEST_ASSERT(SwimLaneText_Utf8BytesForWidth(invalid_utf8, 2u, 1u, &cols) == 1u);
    TEST_ASSERT(cols == 1u);
    TEST_ASSERT(SwimLaneText_Utf8BytesForWidth(incomplete_utf8, 2u, 2u, &cols) == 2u);
    TEST_ASSERT(cols == 2u);
    width_bytes = SwimLaneText_Utf8BytesForWidth(wide_text, sizeof(wide_text), 1u, &cols);
    TEST_ASSERT((width_bytes == 1u) || (width_bytes == sizeof(wide_text)));
    TEST_ASSERT(cols == 1u);
    return 0;
}

/*********************************************************************
*
*       main()
*
*  Function description
*    Run SwimLane renderer unit tests.
*
*  Return value
*    0  All tests passed.
*    1  A test failed.
*/
int main(void) {
    if (_TestNarrowWidthClampsToMinimumLayout() != 0) {
        return 1;
    }
    if (_TestExplicitNarrowWidthFails() != 0) {
        return 1;
    }
    if (_TestNormalWidthUsesAvailableColumns() != 0) {
        return 1;
    }
    if (_TestExplicitOversizedWidthFails() != 0) {
        return 1;
    }
    if (_TestCellAndLayoutBoundaryContracts() != 0) {
        return 1;
    }
    if (_TestPublicRenderLifecycle() != 0) {
        return 1;
    }
    if (_TestRenderEntryAutoHeaderAndColorBranches() != 0) {
        return 1;
    }
    if (_TestTextAnsiAndUtf8Helpers() != 0) {
        return 1;
    }
    return 0;
}

/*************************** End of file ****************************/
