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
File    : SwimLaneRenderer.h
Purpose : Swimlane-style log renderer for heterogeneous system debugging
---------------------------END-OF-HEADER------------------------------
*/

#ifndef TRACEHUB_SWIMLANERENDERER_H            // Guard against multiple inclusion
#define TRACEHUB_SWIMLANERENDERER_H

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "LogEntry.h"

#if defined(__cplusplus)         // Allow usage of this module from C++ files (disable name mangling)
extern "C" {     /* Make sure we have C-declarations in C++ programs */
#endif

/*********************************************************************
*
*       Defines
*
**********************************************************************
*/

/*********************************************************************
*
*       SWIMLANE_DEFAULT_TIMESTAMP_WIDTH
*  Default width of timestamp column in characters.
*
*/
#ifndef SWIMLANE_DEFAULT_TIMESTAMP_WIDTH
  #define SWIMLANE_DEFAULT_TIMESTAMP_WIDTH   12
#endif

/*********************************************************************
*
*       SWIMLANE_DEFAULT_TOTAL_WIDTH
*  Default total renderer width when stdout is not a terminal.
*
*/
#ifndef SWIMLANE_DEFAULT_TOTAL_WIDTH
  #define SWIMLANE_DEFAULT_TOTAL_WIDTH       120
#endif

/*********************************************************************
*
*       SWIMLANE_SEPARATOR_WIDTH
*  Fixed width consumed by inter-column separators.
*
*/
#ifndef SWIMLANE_SEPARATOR_WIDTH
  #define SWIMLANE_SEPARATOR_WIDTH           6
#endif

/*********************************************************************
*
*       SWIMLANE_MIN_TIMESTAMP_WIDTH
*  Minimum timestamp column width accepted by the renderer.
*
*/
#ifndef SWIMLANE_MIN_TIMESTAMP_WIDTH
  #define SWIMLANE_MIN_TIMESTAMP_WIDTH       10
#endif

/*********************************************************************
*
*       SWIMLANE_MIN_SOURCE_WIDTH
*  Minimum per-source column width accepted by the renderer.
*
*/
#ifndef SWIMLANE_MIN_SOURCE_WIDTH
  #define SWIMLANE_MIN_SOURCE_WIDTH          16
#endif

/*********************************************************************
*
*       SWIMLANE_DEFAULT_LINUX_LABEL
*  Default label for the Linux swimlane column.
*
*/
#ifndef SWIMLANE_DEFAULT_LINUX_LABEL
  #define SWIMLANE_DEFAULT_LINUX_LABEL       LOG_SOURCE_DEFAULT_LINUX_LABEL
#endif

/*********************************************************************
*
*       SWIMLANE_DEFAULT_RTOS_LABEL
*  Default label for the RTOS swimlane column.
*
*/
#ifndef SWIMLANE_DEFAULT_RTOS_LABEL
  #define SWIMLANE_DEFAULT_RTOS_LABEL        LOG_SOURCE_DEFAULT_RTOS_LABEL
#endif

/*********************************************************************
*
*       Types
*
**********************************************************************
*/

/*********************************************************************
*
*       SwimLane_Config_t
*
*  Description
*    Configuration structure for swimlane renderer.
*
*  Fields
*    total_width       Total renderer width, 0 for automatic resolution
*    show_header       Show column headers flag
*    show_separator    Show row separator lines flag
*    color_enabled     Enable ANSI color output flag
*    output_stream     Output stream (default stdout)
*/
typedef struct {
    unsigned    total_width;
    bool        show_header;
    bool        show_separator;
    bool        color_enabled;
    FILE       *output_stream;
} SwimLane_Config_t;

/*********************************************************************
*
*       SwimLane_State_t
*
*  Description
*    Runtime state of the swimlane renderer.
*
*  Fields
*    config        Configuration copy
*    initialized   Initialization flag
*    header_shown  Header already shown flag
*    row_count     Number of rows rendered
*/
typedef struct {
    SwimLane_Config_t   config;
    bool                initialized;
    bool                header_shown;
    unsigned            row_count;
} SwimLane_State_t;

/*********************************************************************
*
*       API functions
*
**********************************************************************
*/

int                      SwimLane_Init           (SwimLane_Config_t *config);
void                     SwimLane_Cleanup        (void);
int                      SwimLane_RenderHeader   (void);
int                      SwimLane_RenderSeparator(void);
int                      SwimLane_RenderEntry    (const LogEntry_t *entry);
const SwimLane_State_t * SwimLane_GetState       (void);
unsigned                 SwimLane_GetRowCount    (void);

#if defined(__cplusplus)          // Allow usage of this module from C++ files (disable name mangling)
}                /* Make sure we have C-declarations in C++ programs */
#endif

#endif // end of include guard: TRACEHUB_SWIMLANERENDERER_H Avoid multiple inclusion

/*************************** End of file ****************************/
