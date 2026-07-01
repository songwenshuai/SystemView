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
File    : SwimLaneLayout.c
Purpose : Swimlane terminal and column layout resolution
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

#include "SwimLaneRenderer_internal.h"

#if defined(_WIN32)
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
  #endif
  #include <windows.h>
  #include <io.h>
  #define fileno _fileno
  #define isatty _isatty
#else
  #include <unistd.h>
  #include <sys/ioctl.h>
#endif

/*********************************************************************
*
*       Static functions
*
**********************************************************************
*/

/*********************************************************************
*
*       _AddUnsigned()
*
*  Function description
*    Add two unsigned values with overflow detection.
*/
static int _AddUnsigned(unsigned a, unsigned b, unsigned *result) {
    if (result == NULL) {
        return -1;
    }
    if (a > UINT_MAX - b) {
        return -1;
    }

    *result = a + b;
    return 0;
}

/*********************************************************************
*
*       _GetMinimumTotalWidth()
*
*  Function description
*    Calculate the minimum renderable swimlane total width.
*/
static int _GetMinimumTotalWidth(unsigned timestamp_width, unsigned *min_total_width) {
    unsigned min_source_width;
    unsigned total_width;

    if (min_total_width == NULL) {
        return -1;
    }
    if (_AddUnsigned(SWIMLANE_MIN_SOURCE_WIDTH,
                     SWIMLANE_MIN_SOURCE_WIDTH,
                     &min_source_width) != 0 ||
        _AddUnsigned(timestamp_width,
                     SWIMLANE_SEPARATOR_WIDTH,
                     &total_width) != 0 ||
        _AddUnsigned(total_width,
                     min_source_width,
                     &total_width) != 0) {
        return -1;
    }

    *min_total_width = total_width;
    return 0;
}

/*********************************************************************
*
*       _GetStreamTerminalWidth()
*
*  Function description
*    Read terminal width for a stream when it is connected to a TTY.
*/
static int _GetStreamTerminalWidth(FILE *stream, unsigned *width, bool *is_tty) {
    int fd;

    if (width == NULL || is_tty == NULL) {
        return -1;
    }

    *width = 0u;
    *is_tty = false;
    if (stream == NULL) {
        return 0;
    }

    fd = fileno(stream);
    if (fd < 0 || !isatty(fd)) {
        return 0;
    }
    *is_tty = true;

#if defined(_WIN32)
    {
        CONSOLE_SCREEN_BUFFER_INFO info;
        HANDLE handle;

        handle = (HANDLE)_get_osfhandle(fd);
        if (handle == INVALID_HANDLE_VALUE ||
            !GetConsoleScreenBufferInfo(handle, &info)) {
            return 0;
        }
        *width = (unsigned)(info.srWindow.Right - info.srWindow.Left + 1);
        return 0;
    }
#else
    {
        struct winsize ws;

        if (ioctl(fd, TIOCGWINSZ, &ws) != 0 || ws.ws_col == 0u) {
            return 0;
        }
        *width = (unsigned)ws.ws_col;
        return 0;
    }
#endif
}

/*********************************************************************
*
*       Public internal functions
*
**********************************************************************
*/

/*********************************************************************
*
*       SwimLaneLayout_ResolveColumnWidths()
*
*  Function description
*    Resolve Linux and RTOS column widths from total width. Abnormally narrow
*    terminal widths are clamped to the minimum renderable layout.
*/
int SwimLaneLayout_ResolveColumnWidths(unsigned total_width,
                                       unsigned timestamp_width,
                                       bool clamp_narrow,
                                       unsigned *linux_width,
                                       unsigned *rtos_width) {
    unsigned min_total_width;
    unsigned source_width;

    if (linux_width == NULL || rtos_width == NULL) {
        return -1;
    }
    if (_GetMinimumTotalWidth(timestamp_width, &min_total_width) != 0) {
        return -1;
    }
    if (total_width < min_total_width) {
        if (!clamp_narrow) {
            return -1;
        }
        total_width = min_total_width;
    }

    source_width = total_width - timestamp_width - SWIMLANE_SEPARATOR_WIDTH;
    *linux_width = source_width / 2u;
    *rtos_width = source_width - *linux_width;

    if (*linux_width < SWIMLANE_MIN_SOURCE_WIDTH ||
        *rtos_width < SWIMLANE_MIN_SOURCE_WIDTH) {
        return -1;
    }

    return 0;
}

/*********************************************************************
*
*       SwimLaneLayout_Resolve()
*
*  Function description
*    Resolve renderer layout from the output stream, explicit configuration,
*    and compile-time defaults.
*/
int SwimLaneLayout_Resolve(const SwimLane_Config_t *config, SwimLane_Layout_t *layout) {
    unsigned terminal_width;
    unsigned total_width;
    bool     explicit_width;
    bool     clamp_narrow;
    bool     is_tty;

    if (config == NULL || layout == NULL) {
        return -1;
    }
    if (SWIMLANE_DEFAULT_TIMESTAMP_WIDTH < SWIMLANE_MIN_TIMESTAMP_WIDTH) {
        return -1;
    }
    if (_GetStreamTerminalWidth(config->output_stream, &terminal_width, &is_tty) != 0) {
        return -1;
    }

    explicit_width = config->total_width != 0u;
    if (explicit_width) {
        total_width = config->total_width;
    } else {
        total_width = (terminal_width > 0u) ?
                      terminal_width :
                      SWIMLANE_DEFAULT_TOTAL_WIDTH;
    }
    if (total_width > (unsigned)INT_MAX) {
        return -1;
    }
    clamp_narrow = !explicit_width && (terminal_width > 0u);

    layout->timestamp_width = SWIMLANE_DEFAULT_TIMESTAMP_WIDTH;
    if (SwimLaneLayout_ResolveColumnWidths(total_width,
                                           layout->timestamp_width,
                                           clamp_narrow,
                                           &layout->linux_width,
                                           &layout->rtos_width) != 0) {
        return -1;
    }
    layout->linux_label = SWIMLANE_DEFAULT_LINUX_LABEL;
    layout->rtos_label  = SWIMLANE_DEFAULT_RTOS_LABEL;
    layout->stream_is_tty = is_tty;

    return 0;
}

/*************************** End of file ****************************/
