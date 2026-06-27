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
File    : SwimLaneRenderer.c
Purpose : Swimlane-style log renderer implementation
---------------------------END-OF-HEADER------------------------------
*/

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

//
// Enable POSIX extensions for wcwidth() declaration
// Must be defined before any system headers
//
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <wchar.h>
#include <locale.h>

#include "SwimLaneRenderer.h"
#include "LogEntry.h"
#include "SYS.h"

/*********************************************************************
*
*       Defines
*
**********************************************************************
*/

/*********************************************************************
*
*       ANSI Color Codes
*
*  These codes enable colored output in terminal emulators.
*  Format: ESC[<code>m
*
**********************************************************************
*/
#define ANSI_COLOR_RESET      "\033[0m"
#define ANSI_COLOR_TIMESTAMP  "\033[36m"    // Cyan for timestamps
#define ANSI_COLOR_LINUX      "\033[32m"    // Green for Linux
#define ANSI_COLOR_RTOS       "\033[33m"    // Yellow for RTOS
#define ANSI_COLOR_HEADER     "\033[1;37m"  // Bold white for headers
#define ANSI_COLOR_SEPARATOR  "\033[90m"    // Dark gray for separators

#define ANSI_ESC              '\033'
#define ANSI_BEL              '\007'
#define ANSI_MAX_SEQUENCE_LEN 128u

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

static SwimLane_State_t _swimlane_state;
static SYS_Mutex        _swimlane_mutex;
static bool             _mutex_initialized = false;

/*********************************************************************
*
*       Static functions
*
**********************************************************************
*/

/*********************************************************************
*
*       _AnsiEscapeLength()
*
*  Function description
*    Return the byte length of one complete ANSI escape sequence.
*/
static unsigned _AnsiEscapeLength(const char *str) {
    unsigned i;

    if (str == NULL || str[0] != ANSI_ESC || str[1] == '\0') {
        return 0u;
    }

    if (str[1] == '[') {
        i = 2u;
        while (str[i] != '\0' && i < ANSI_MAX_SEQUENCE_LEN) {
            unsigned char c = (unsigned char)str[i];

            i++;
            if (c >= 0x40u && c <= 0x7Eu) {
                return i;
            }
        }
        return 0u;
    }

    if (str[1] == ']') {
        i = 2u;
        while (str[i] != '\0' && i < ANSI_MAX_SEQUENCE_LEN) {
            if (str[i] == ANSI_BEL) {
                return i + 1u;
            }
            if (str[i] == ANSI_ESC && str[i + 1u] == '\\') {
                return i + 2u;
            }
            i++;
        }
        return 0u;
    }

    if ((unsigned char)str[1] >= 0x40u && (unsigned char)str[1] <= 0x5Fu) {
        return 2u;
    }

    return 0u;
}

/*********************************************************************
*
*       _WriteCellBytes()
*
*  Function description
*    Write a source byte range to the output stream. When color output is
*    disabled, ANSI escape sequences are omitted while all visible bytes are
*    preserved.
*/
static void _WriteCellBytes(FILE *stream, const char *src, unsigned src_bytes) {
    unsigned pos;

    if (stream == NULL || src == NULL || src_bytes == 0u) {
        return;
    }

    if (_swimlane_state.config.color_enabled) {
        (void)fwrite(src, 1u, src_bytes, stream);
        return;
    }

    pos = 0u;
    while (pos < src_bytes) {
        unsigned ansi_len;

        ansi_len = _AnsiEscapeLength(src + pos);
        if (ansi_len > 0u && ansi_len <= src_bytes - pos) {
            pos += ansi_len;
            continue;
        }

        (void)fwrite(src + pos, 1u, 1u, stream);
        pos++;
    }
}

/*********************************************************************
*
*       _Utf8BytesForWidth()
*
*  Function description
*    Calculate the number of bytes from a UTF-8 string that fit within
*    a specified display column width. Uses mbrtowc + wcwidth to correctly
*    handle CJK full-width characters (2 columns), ASCII (1 column), and
*    ANSI escape sequences (0 columns).
*
*  Parameters
*    str       UTF-8 encoded string
*    max_bytes Maximum number of source bytes available
*    max_cols  Maximum display columns allowed
*    out_cols  Optional output: actual display columns consumed
*
*  Return value
*    Number of bytes that fit within max_cols display width
*/
static unsigned _Utf8BytesForWidth(const char *str, unsigned max_bytes,
                                   unsigned max_cols, unsigned *out_cols) {
    mbstate_t   state;
    unsigned    cols = 0;
    unsigned    bytes = 0;
    const char *p = str;
    //
    // Handle NULL or empty string
    //
    if (out_cols != NULL) {
        *out_cols = 0;
    }
    if (str == NULL || max_bytes == 0u || max_cols == 0u) {
        return 0;
    }
    //
    // Initialize multibyte conversion state
    //
    memset(&state, 0, sizeof(state));
    //
    // Process characters until we reach max_cols or end of string
    //
    while (bytes < max_bytes && cols < max_cols) {
        wchar_t wc;
        size_t  n;
        int     w;
        unsigned ansi_len;
        //
        // Target-side ANSI sequences affect terminal state but have no visible width.
        // They are consumed for wrapping and emitted or filtered by _PrintCell().
        //
        ansi_len = _AnsiEscapeLength(p);
        if (ansi_len > 0u && ansi_len <= max_bytes - bytes) {
            p += ansi_len;
            bytes += ansi_len;
            continue;
        }
        //
        // Convert next multibyte character to wide character
        //
        n = mbrtowc(&wc, p, max_bytes - bytes, &state);
        if (n == (size_t)-1 || n == (size_t)-2) {
            //
            // Invalid or incomplete UTF-8 sequence: treat as single byte with width 1
            //
            memset(&state, 0, sizeof(state));
            if (cols + 1 > max_cols) {
                break;
            }
            p++;
            bytes++;
            cols++;
            continue;
        }
        if (n == 0) {
            //
            // Null character encountered
            //
            break;
        }
        if (n > max_bytes - bytes) {
            break;
        }
        //
        // Get display width of the character
        //
        w = wcwidth(wc);
        if (w < 0) {
            //
            // Non-printable control character: treat as width 1
            //
            w = 1;
        }
        //
        // Check if this character fits
        //
        if (cols + (unsigned)w > max_cols) {
            //
            // Character doesn't fit. To avoid infinite loops with wide
            // characters when max_cols is too small, consume it if nothing
            // has been emitted yet.
            //
            if (bytes == 0) {
                p += n;
                bytes += (unsigned)n;
                cols = max_cols;
            }
            break;
        }
        //
        // Character fits, advance
        //
        p += n;
        bytes += (unsigned)n;
        cols += (unsigned)w;
    }
    while (bytes < max_bytes) {
        unsigned ansi_len;

        ansi_len = _AnsiEscapeLength(p);
        if (ansi_len == 0u || ansi_len > max_bytes - bytes) {
            break;
        }
        p += ansi_len;
        bytes += ansi_len;
    }
    //
    // Return actual columns consumed if requested
    //
    if (out_cols != NULL) {
        *out_cols = cols;
    }
    return bytes;
}

/*********************************************************************
*
*       _PrintCell()
*
*  Function description
*    Print a table cell with fixed display column width and padding.
*    Content is left-aligned. Uses display width calculation to correctly
*    handle CJK full-width characters (2 columns each).
*
*  Parameters
*    stream     Output stream
*    content    Cell content string (UTF-8 encoded)
*    width      Fixed cell width in display columns
*    color      ANSI color code (or NULL for no color)
*    offset     Starting byte offset into content string
*
*  Return value
*    Number of content bytes printed (for tracking offset)
*/
static unsigned _PrintCell(FILE *stream, const char *content, unsigned width,
                           const char *color, unsigned offset) {
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
    // Get actual byte length (without trailing newlines)
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
    // Calculate how many bytes fit within the display width
    // This correctly handles CJK characters (2 columns) vs ASCII (1 column)
    //
    print_bytes = _Utf8BytesForWidth(start, content_len - offset, width, &print_cols);
    //
    // Safety: ensure forward progress for non-empty content
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
    if (color != NULL && _swimlane_state.config.color_enabled) {
        fprintf(stream, "%s", color);
    }
    //
    // Print the selected byte range. ANSI control sequences have zero display width.
    //
    _WriteCellBytes(stream, start, print_bytes);
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
    if (_swimlane_state.config.color_enabled) {
        fprintf(stream, "%s", ANSI_COLOR_RESET);
    }

    return print_bytes;
}

/*********************************************************************
*
*       _PrintTimestampCell()
*
*  Function description
*    Print timestamp cell with HH:MM:SS.mmm format converted from microseconds.
*
*  Parameters
*    stream        Output stream
*    timestamp_us  Timestamp in microseconds
*    width         Cell width
*    show_time     If true, show timestamp; if false, show empty cell
*/
static void _PrintTimestampCell(FILE *stream, uint64_t timestamp_us, unsigned width, bool show_time) {
    char ts_buf[32];
    unsigned hours, minutes, seconds, milliseconds;
    uint64_t total_seconds;
    //
    // Handle continuation rows (no timestamp)
    //
    if (!show_time) {
        _PrintCell(stream, "", width, NULL, 0);
        return;
    }
    //
    // Convert microseconds back to HH:MM:SS.mmm format for display
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
    _PrintCell(stream, ts_buf, width, ANSI_COLOR_TIMESTAMP, 0);
}

/*********************************************************************
*
*       Public functions
*
**********************************************************************
*/

/*********************************************************************
*
*       SwimLane_Init()
*
*  Function description
*    Initialize the swimlane renderer with the specified configuration.
*
*  Parameters
*    config  Pointer to configuration structure
*
*  Return value
*    0   Success
*   -1   Invalid configuration or already initialized
*   -2   Failed to initialize mutex
*/
int SwimLane_Init(SwimLane_Config_t *config) {
    unsigned total_width;
    //
    // Validate configuration
    //
    if (config == NULL) {
        return -1;
    }
    if (_swimlane_state.initialized || _mutex_initialized) {
        return -1;
    }
    if (config->timestamp_width == 0 || config->linux_width == 0 || config->rtos_width == 0) {
        return -1;
    }
    if (config->timestamp_width > (unsigned)INT_MAX ||
        config->linux_width > (unsigned)INT_MAX ||
        config->rtos_width > (unsigned)INT_MAX) {
        return -1;
    }
    if (config->timestamp_width > (unsigned)INT_MAX - 6u) {
        return -1;
    }
    total_width = config->timestamp_width + 6u;
    if (config->linux_width > (unsigned)INT_MAX - total_width) {
        return -1;
    }
    total_width += config->linux_width;
    if (config->rtos_width > (unsigned)INT_MAX - total_width) {
        return -1;
    }
    //
    // Initialize mutex for thread safety (multiple collectors may call RenderEntry)
    //
    if (SYS_MutexInit(&_swimlane_mutex) != 0) {
        return -2;
    }
    _mutex_initialized = true;
    //
    // Initialize state
    //
    memset(&_swimlane_state, 0, sizeof(_swimlane_state));
    memcpy(&_swimlane_state.config, config, sizeof(SwimLane_Config_t));
    //
    // Set default output stream if not specified
    //
    if (_swimlane_state.config.output_stream == NULL) {
        _swimlane_state.config.output_stream = stdout;
    }
    //
    // Mark as initialized
    //
    _swimlane_state.initialized  = true;
    _swimlane_state.header_shown = false;
    _swimlane_state.row_count    = 0;

    return 0;
}

/*********************************************************************
*
*       SwimLane_Cleanup()
*
*  Function description
*    Cleanup swimlane renderer resources.
*/
void SwimLane_Cleanup(void) {
    //
    // Flush output stream
    //
    if (_swimlane_state.initialized && _swimlane_state.config.output_stream != NULL) {
        fflush(_swimlane_state.config.output_stream);
    }
    //
    // Reset state
    //
    memset(&_swimlane_state, 0, sizeof(_swimlane_state));
    //
    // Destroy mutex
    //
    if (_mutex_initialized) {
        SYS_MutexDestroy(&_swimlane_mutex);
        _mutex_initialized = false;
    }
}

/*********************************************************************
*
*       _SwimLane_RenderHeaderUnlocked()
*
*  Function description
*    Render the swimlane header with column titles.
*    This is called automatically by SwimLane_RenderEntry() if needed.
*
*  Return value
*    0   Success
*   -1   Renderer not initialized
*/
static int _SwimLane_RenderHeaderUnlocked(void) {
    FILE     *stream;
    unsigned  ts_width;
    unsigned  linux_width;
    unsigned  rtos_width;
    unsigned  total_width;
    unsigned  i;
    //
    // Check initialization
    //
    if (!_swimlane_state.initialized) {
        return -1;
    }
    //
    // Skip if header already shown
    //
    if (_swimlane_state.header_shown) {
        return 0;
    }
    //
    // Get configuration
    //
    stream       = _swimlane_state.config.output_stream;
    ts_width     = _swimlane_state.config.timestamp_width;
    linux_width  = _swimlane_state.config.linux_width;
    rtos_width   = _swimlane_state.config.rtos_width;
    total_width  = ts_width + 3 + linux_width + 3 + rtos_width;
    //
    // Print top separator
    //
    if (_swimlane_state.config.show_separator) {
        if (_swimlane_state.config.color_enabled) {
            fprintf(stream, "%s", ANSI_COLOR_SEPARATOR);
        }
        for (i = 0; i < total_width; i++) {
            fprintf(stream, "-");
        }
        fprintf(stream, "\n");
        if (_swimlane_state.config.color_enabled) {
            fprintf(stream, "%s", ANSI_COLOR_RESET);
        }
    }
    //
    // Print header row
    //
    if (_swimlane_state.config.show_header) {
        if (_swimlane_state.config.color_enabled) {
            fprintf(stream, "%s", ANSI_COLOR_HEADER);
        }
        //
        // Timestamp column header
        //
        _PrintCell(stream, "TIME", ts_width, NULL, 0);
        fprintf(stream, " | ");
        //
        // Linux column header
        //
        _PrintCell(stream, "LINUX CORE (A53)", linux_width, NULL, 0);
        fprintf(stream, " | ");
        //
        // RTOS column header
        //
        _PrintCell(stream, "RTOS CORE (R5)", rtos_width, NULL, 0);
        fprintf(stream, "\n");
        //
        // Reset color
        //
        if (_swimlane_state.config.color_enabled) {
            fprintf(stream, "%s", ANSI_COLOR_RESET);
        }
    }
    //
    // Print header separator
    //
    if (_swimlane_state.config.show_separator) {
        if (_swimlane_state.config.color_enabled) {
            fprintf(stream, "%s", ANSI_COLOR_SEPARATOR);
        }
        for (i = 0; i < total_width; i++) {
            fprintf(stream, "-");
        }
        fprintf(stream, "\n");
        if (_swimlane_state.config.color_enabled) {
            fprintf(stream, "%s", ANSI_COLOR_RESET);
        }
    }
    //
    // Flush output
    //
    fflush(stream);
    //
    // Mark header as shown
    //
    _swimlane_state.header_shown = true;

    return 0;
}

/*********************************************************************
*
*       SwimLane_RenderHeader()
*
*  Function description
*    Render the swimlane header with column titles.
*    This is called automatically by SwimLane_RenderEntry() if needed.
*
*  Return value
*    0   Success
*   -1   Renderer not initialized
*/
int SwimLane_RenderHeader(void) {
    int Result;

    if (_mutex_initialized) {
        SYS_MutexLock(&_swimlane_mutex);
    }
    Result = _SwimLane_RenderHeaderUnlocked();
    if (_mutex_initialized) {
        SYS_MutexUnlock(&_swimlane_mutex);
    }

    return Result;
}

/*********************************************************************
*
*       _SwimLane_RenderSeparatorUnlocked()
*
*  Function description
*    Render a horizontal separator line.
*
*  Return value
*    0   Success
*   -1   Renderer not initialized
*/
static int _SwimLane_RenderSeparatorUnlocked(void) {
    FILE     *stream;
    unsigned  total_width;
    unsigned  i;
    //
    // Check initialization
    //
    if (!_swimlane_state.initialized) {
        return -1;
    }
    //
    // Get configuration
    //
    stream      = _swimlane_state.config.output_stream;
    total_width = _swimlane_state.config.timestamp_width + 3 +
                  _swimlane_state.config.linux_width + 3 +
                  _swimlane_state.config.rtos_width;
    //
    // Print separator line
    //
    if (_swimlane_state.config.color_enabled) {
        fprintf(stream, "%s", ANSI_COLOR_SEPARATOR);
    }
    for (i = 0; i < total_width; i++) {
        fprintf(stream, "-");
    }
    fprintf(stream, "\n");
    if (_swimlane_state.config.color_enabled) {
        fprintf(stream, "%s", ANSI_COLOR_RESET);
    }
    //
    // Flush output
    //
    fflush(stream);

    return 0;
}

/*********************************************************************
*
*       SwimLane_RenderSeparator()
*
*  Function description
*    Render a horizontal separator line.
*
*  Return value
*    0   Success
*   -1   Renderer not initialized
*/
int SwimLane_RenderSeparator(void) {
    int Result;

    if (_mutex_initialized) {
        SYS_MutexLock(&_swimlane_mutex);
    }
    Result = _SwimLane_RenderSeparatorUnlocked();
    if (_mutex_initialized) {
        SYS_MutexUnlock(&_swimlane_mutex);
    }

    return Result;
}

/*********************************************************************
*
*       SwimLane_RenderEntry()
*
*  Function description
*    Render a single log entry in swimlane format.
*    The entry will be placed in the appropriate column based on its source.
*
*  Parameters
*    entry  Pointer to LogEntry_t to render
*
*  Return value
*    0   Success
*   -1   Renderer not initialized
*   -2   Invalid entry
*/
int SwimLane_RenderEntry(const LogEntry_t *entry) {
    FILE        *stream;
    uint64_t     timestamp;
    LogSource_t  source;
    const char  *content;
    unsigned     ts_width;
    unsigned     linux_width;
    unsigned     rtos_width;
    unsigned     content_len;
    unsigned     offset;
    unsigned     printed;
    unsigned     rows_rendered;
    bool         first_line;
    bool         show_timestamp;
    //
    // Check initialization
    //
    if (!_swimlane_state.initialized) {
        return -1;
    }
    //
    // Validate entry
    //
    if (!LogEntry_IsValid(entry)) {
        return -2;
    }
    //
    // Lock mutex for thread-safe rendering (multiple collectors may call this)
    //
    if (_mutex_initialized) {
        SYS_MutexLock(&_swimlane_mutex);
    }
    //
    // Show header if not shown yet
    //
    if (!_swimlane_state.header_shown) {
        _SwimLane_RenderHeaderUnlocked();
    }
    //
    // Get configuration
    //
    stream       = _swimlane_state.config.output_stream;
    ts_width     = _swimlane_state.config.timestamp_width;
    linux_width  = _swimlane_state.config.linux_width;
    rtos_width   = _swimlane_state.config.rtos_width;
    //
    // Get entry fields
    //
    timestamp = LogEntry_GetTimestamp(entry);
    source    = LogEntry_GetSource(entry);
    content   = LogEntry_GetContent(entry);
    //
    // Calculate content length (excluding trailing newlines)
    //
    content_len = (content != NULL) ? (unsigned)strlen(content) : 0;
    while (content_len > 0 && (content[content_len - 1] == '\n' || content[content_len - 1] == '\r')) {
        content_len--;
    }
    //
    // Print content with line wrapping
    //
    offset = 0;
    rows_rendered = 0;
    first_line = true;
    do {
        //
        // Print timestamp column (only on first line)
        //
        show_timestamp = first_line && !LogEntry_IsFragmentContinuation(entry);
        _PrintTimestampCell(stream, timestamp, ts_width, show_timestamp);
        fprintf(stream, " | ");
        //
        // Print Linux or RTOS column based on source
        //
        if (source == LOG_SOURCE_LINUX) {
            //
            // Linux column: print content with target-side ANSI preserved
            //
            printed = _PrintCell(stream, content, linux_width, NULL, offset);
            fprintf(stream, " | ");
            //
            // RTOS column: empty
            //
            _PrintCell(stream, "", rtos_width, NULL, 0);
        } else {
            //
            // Linux column: empty
            //
            _PrintCell(stream, "", linux_width, NULL, 0);
            fprintf(stream, " | ");
            //
            // RTOS column: print content with target-side ANSI preserved
            //
            printed = _PrintCell(stream, content, rtos_width, NULL, offset);
        }
        fprintf(stream, "\n");
        rows_rendered++;
        //
        // Move to next line segment
        //
        offset += printed;
        first_line = false;
        //
        // Continue if there's more content
        //
    } while (offset < content_len && printed > 0);
    //
    // Flush output
    //
    fflush(stream);
    //
    // Increment row count
    //
    _swimlane_state.row_count += rows_rendered;
    //
    // Unlock mutex
    //
    if (_mutex_initialized) {
        SYS_MutexUnlock(&_swimlane_mutex);
    }

    return 0;
}

/*********************************************************************
*
*       SwimLane_GetState()
*
*  Function description
*    Get pointer to the renderer state structure.
*
*  Return value
*    Read-only pointer to SwimLane_State_t structure, or NULL if not initialized
*/
const SwimLane_State_t* SwimLane_GetState(void) {
    if (!_swimlane_state.initialized) {
        return NULL;
    }
    return &_swimlane_state;
}

/*********************************************************************
*
*       SwimLane_GetRowCount()
*
*  Function description
*    Get the number of rows rendered so far.
*
*  Return value
*    Number of rows rendered
*/
unsigned SwimLane_GetRowCount(void) {
    return _swimlane_state.row_count;
}

/*************************** End of file ****************************/
