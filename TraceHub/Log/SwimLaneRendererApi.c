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
File    : SwimLaneRendererApi.c
Purpose : Swimlane-style log renderer implementation
---------------------------END-OF-HEADER------------------------------
*/

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include <stdio.h>
#include <string.h>

#include "SwimLaneRenderer_internal.h"

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

SwimLane_State_t  _swimlane_state;
SYS_Mutex         _swimlane_mutex;
bool              _swimlane_mutex_initialized = false;
SwimLane_Layout_t _swimlane_layout;

/*********************************************************************
*
*       Static functions
*
**********************************************************************
*/

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
    stream      = _swimlane_state.config.output_stream;
    ts_width    = _swimlane_layout.timestamp_width;
    linux_width = _swimlane_layout.linux_width;
    rtos_width  = _swimlane_layout.rtos_width;
    total_width = ts_width + 3 + linux_width + 3 + rtos_width;
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
        SwimLaneCell_Print(stream, "TIME", ts_width, NULL, 0, _swimlane_state.config.color_enabled);
        fprintf(stream, " | ");
        //
        // Linux column header
        //
        SwimLaneCell_Print(stream, _swimlane_layout.linux_label, linux_width, NULL, 0, _swimlane_state.config.color_enabled);
        fprintf(stream, " | ");
        //
        // RTOS column header
        //
        SwimLaneCell_Print(stream, _swimlane_layout.rtos_label, rtos_width, NULL, 0, _swimlane_state.config.color_enabled);
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
    total_width = _swimlane_layout.timestamp_width + 3 +
                  _swimlane_layout.linux_width + 3 +
                  _swimlane_layout.rtos_width;
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
    //
    // Validate configuration
    //
    if (config == NULL) {
        return -1;
    }
    if (_swimlane_state.initialized || _swimlane_mutex_initialized) {
        return -1;
    }
    //
    // Initialize mutex for thread safety.
    //
    if (SYS_MutexInit(&_swimlane_mutex) != 0) {
        return -2;
    }
    _swimlane_mutex_initialized = true;
    //
    // Initialize state
    //
    memset(&_swimlane_state, 0, sizeof(_swimlane_state));
    memset(&_swimlane_layout, 0, sizeof(_swimlane_layout));
    memcpy(&_swimlane_state.config, config, sizeof(SwimLane_Config_t));
    if (_swimlane_state.config.output_stream == NULL) {
        _swimlane_state.config.output_stream = stdout;
    }
    if (SwimLaneLayout_Resolve(&_swimlane_state.config, &_swimlane_layout) != 0) {
        SYS_MutexDestroy(&_swimlane_mutex);
        _swimlane_mutex_initialized = false;
        memset(&_swimlane_state, 0, sizeof(_swimlane_state));
        memset(&_swimlane_layout, 0, sizeof(_swimlane_layout));
        return -1;
    }
    _swimlane_state.config.color_enabled =
        _swimlane_state.config.color_enabled && _swimlane_layout.stream_is_tty;
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
    memset(&_swimlane_layout, 0, sizeof(_swimlane_layout));
    //
    // Destroy mutex
    //
    if (_swimlane_mutex_initialized) {
        SYS_MutexDestroy(&_swimlane_mutex);
        _swimlane_mutex_initialized = false;
    }
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

    if (_swimlane_mutex_initialized) {
        SYS_MutexLock(&_swimlane_mutex);
    }
    Result = _SwimLane_RenderHeaderUnlocked();
    if (_swimlane_mutex_initialized) {
        SYS_MutexUnlock(&_swimlane_mutex);
    }

    return Result;
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

    if (_swimlane_mutex_initialized) {
        SYS_MutexLock(&_swimlane_mutex);
    }
    Result = _SwimLane_RenderSeparatorUnlocked();
    if (_swimlane_mutex_initialized) {
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
    bool         color_enabled;
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
    // Lock mutex for thread-safe rendering.
    //
    if (_swimlane_mutex_initialized) {
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
    stream        = _swimlane_state.config.output_stream;
    ts_width      = _swimlane_layout.timestamp_width;
    linux_width   = _swimlane_layout.linux_width;
    rtos_width    = _swimlane_layout.rtos_width;
    color_enabled = _swimlane_state.config.color_enabled;
    //
    // Get entry fields
    //
    timestamp = LogEntry_GetTimestamp(entry);
    source    = LogEntry_GetSource(entry);
    content   = LogEntry_GetContent(entry);
    //
    // Calculate content length excluding trailing newlines.
    //
    content_len = (content != NULL) ? (unsigned)strlen(content) : 0;
    while (content_len > 0 && (content[content_len - 1] == '\n' || content[content_len - 1] == '\r')) {
        content_len--;
    }
    //
    // Print content with line wrapping.
    //
    offset = 0;
    rows_rendered = 0;
    first_line = true;
    do {
        //
        // Print timestamp column only on the first line.
        //
        show_timestamp = first_line && !LogEntry_IsFragmentContinuation(entry);
        SwimLaneCell_PrintTimestamp(stream, timestamp, ts_width, show_timestamp, color_enabled);
        fprintf(stream, " | ");
        //
        // Print the column selected by the entry source.
        //
        if (source == LOG_SOURCE_LINUX) {
            //
            // Linux column: print content with target-side ANSI preserved
            //
            printed = SwimLaneCell_Print(stream, content, linux_width, NULL, offset, color_enabled);
            fprintf(stream, " | ");
            //
            // RTOS column: empty
            //
            SwimLaneCell_Print(stream, "", rtos_width, NULL, 0, color_enabled);
        } else {
            //
            // Linux column: empty
            //
            SwimLaneCell_Print(stream, "", linux_width, NULL, 0, color_enabled);
            fprintf(stream, " | ");
            //
            // RTOS column: print content with target-side ANSI preserved
            //
            printed = SwimLaneCell_Print(stream, content, rtos_width, NULL, offset, color_enabled);
        }
        fprintf(stream, "\n");
        rows_rendered++;
        //
        // Move to next line segment
        //
        offset += printed;
        first_line = false;
        //
        // Continue if there is more content
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
    if (_swimlane_mutex_initialized) {
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
