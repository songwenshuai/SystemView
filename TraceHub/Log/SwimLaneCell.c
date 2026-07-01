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
File    : SwimLaneCell.c
Purpose : Swimlane fixed-width cell rendering
---------------------------END-OF-HEADER------------------------------
*/

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include <limits.h>
#include <string.h>

#include "SwimLaneRenderer_internal.h"

/*********************************************************************
*
*       Public internal functions
*
**********************************************************************
*/

/*********************************************************************
*
*       SwimLaneCell_Print()
*
*  Function description
*    Print a table cell with fixed display column width and padding.
*    Content is left-aligned. Uses display width calculation to correctly
*    handle CJK full-width characters.
*
*  Parameters
*    stream         Output stream
*    content        Cell content string
*    width          Fixed cell width in display columns
*    color          ANSI color code, or NULL for no color
*    offset         Starting byte offset into content string
*    color_enabled  Enable ANSI color output
*
*  Return value
*    Number of content bytes printed.
*/
unsigned SwimLaneCell_Print(FILE *stream, const char *content, unsigned width,
                            const char *color, unsigned offset, bool color_enabled) {
    unsigned    content_len;
    unsigned    print_bytes;
    unsigned    print_cols;
    unsigned    padding;
    const char *start;
    int         field_width;

    if (width > (unsigned)INT_MAX) {
        return 0;
    }
    field_width = (int)width;
    //
    // Handle NULL or empty content
    //
    if (content == NULL || *content == '\0') {
        fprintf(stream, "%-*s", field_width, "");
        return 0;
    }
    //
    // Get actual byte length without trailing newlines.
    //
    content_len = (unsigned)strlen(content);
    while (content_len > 0 && (content[content_len - 1] == '\n' || content[content_len - 1] == '\r')) {
        content_len--;
    }
    //
    // Check if offset is beyond content
    //
    if (offset >= content_len) {
        fprintf(stream, "%-*s", field_width, "");
        return 0;
    }
    //
    // Calculate remaining content starting position
    //
    start = content + offset;
    //
    // Calculate how many bytes fit within the display width.
    //
    print_bytes = SwimLaneText_Utf8BytesForWidth(start, content_len - offset, width, &print_cols);
    //
    // Safety: ensure forward progress for non-empty content.
    //
    if (print_bytes == 0 && *start != '\0') {
        print_bytes = 1;
        print_cols = 1;
    }
    if (print_bytes > (unsigned)INT_MAX) {
        return 0;
    }
    //
    // Apply color if enabled and provided
    //
    if (color != NULL && color_enabled) {
        fprintf(stream, "%s", color);
    }
    //
    // Print the selected byte range. ANSI control sequences have zero display width.
    //
    SwimLaneText_WriteBytes(stream, start, print_bytes, color_enabled);
    //
    // Add padding based on display columns used, not bytes
    //
    if (print_cols < width) {
        padding = width - print_cols;
        fprintf(stream, "%-*s", (int)padding, "");
    }
    //
    // Reset color after both renderer colors and target-side ANSI sequences.
    //
    if (color_enabled) {
        fprintf(stream, "%s", ANSI_COLOR_RESET);
    }

    return print_bytes;
}

/*********************************************************************
*
*       SwimLaneCell_PrintTimestamp()
*
*  Function description
*    Print timestamp cell with HH:MM:SS.mmm format converted from
*    microseconds.
*
*  Parameters
*    stream         Output stream
*    timestamp_us   Timestamp in microseconds
*    width          Cell width
*    show_time      If true, show timestamp; if false, show empty cell
*    color_enabled  Enable ANSI color output
*/
void SwimLaneCell_PrintTimestamp(FILE *stream, uint64_t timestamp_us,
                                 unsigned width, bool show_time,
                                 bool color_enabled) {
    char     ts_buf[32];
    unsigned hours;
    unsigned minutes;
    unsigned seconds;
    unsigned milliseconds;
    uint64_t total_seconds;
    //
    // Handle continuation rows without a timestamp.
    //
    if (!show_time) {
        SwimLaneCell_Print(stream, "", width, NULL, 0, color_enabled);
        return;
    }
    //
    // Convert microseconds back to HH:MM:SS.mmm format for display.
    //
    total_seconds = timestamp_us / 1000000ULL;
    milliseconds = (unsigned)((timestamp_us % 1000000ULL) / 1000ULL);
    hours = (unsigned)(total_seconds / 3600);
    minutes = (unsigned)((total_seconds % 3600) / 60);
    seconds = (unsigned)(total_seconds % 60);
    //
    // Format timestamp
    //
    snprintf(ts_buf, sizeof(ts_buf), "%02u:%02u:%02u.%03u", hours, minutes, seconds, milliseconds);
    //
    // Print with color
    //
    SwimLaneCell_Print(stream, ts_buf, width, ANSI_COLOR_TIMESTAMP, 0, color_enabled);
}

/*************************** End of file ****************************/
