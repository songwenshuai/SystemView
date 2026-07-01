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
    TEST_ASSERT(SwimLane_RenderHeader() == 0);
    TEST_ASSERT(state->header_shown);
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
    if (_TestPublicRenderLifecycle() != 0) {
        return 1;
    }
    return 0;
}

/*************************** End of file ****************************/
