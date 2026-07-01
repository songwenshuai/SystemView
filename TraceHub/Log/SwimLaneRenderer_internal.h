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
File    : SwimLaneRenderer_internal.h
Purpose : Internal swimlane renderer state and function contracts
---------------------------END-OF-HEADER------------------------------
*/

#ifndef TRACEHUB_SWIMLANERENDERER_INTERNAL_H
#define TRACEHUB_SWIMLANERENDERER_INTERNAL_H

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "SwimLaneRenderer.h"
#include "SYS.h"

#if defined(__cplusplus)
extern "C" {
#endif

/*********************************************************************
*
*       Defines
*
**********************************************************************
*/

#define ANSI_COLOR_RESET      "\033[0m"
#define ANSI_COLOR_TIMESTAMP  "\033[36m"
#define ANSI_COLOR_LINUX      "\033[32m"
#define ANSI_COLOR_RTOS       "\033[33m"
#define ANSI_COLOR_HEADER     "\033[1;37m"
#define ANSI_COLOR_SEPARATOR  "\033[90m"

#define ANSI_ESC              '\033'
#define ANSI_BEL              '\007'
#define ANSI_MAX_SEQUENCE_LEN 128u

/*********************************************************************
*
*       Types
*
**********************************************************************
*/

/*********************************************************************
*
*       SwimLane_Layout_t
*
*  Description
*    Resolved swimlane layout facts derived from configuration and stream.
*/
typedef struct {
    unsigned    timestamp_width;
    unsigned    linux_width;
    unsigned    rtos_width;
    const char *linux_label;
    const char *rtos_label;
    bool        stream_is_tty;
} SwimLane_Layout_t;

/*********************************************************************
*
*       Shared state
*
**********************************************************************
*/

extern SwimLane_State_t  _swimlane_state;
extern SYS_Mutex         _swimlane_mutex;
extern bool              _swimlane_mutex_initialized;
extern SwimLane_Layout_t _swimlane_layout;

/*********************************************************************
*
*       Layout functions
*
**********************************************************************
*/

int SwimLaneLayout_ResolveColumnWidths(unsigned total_width, unsigned timestamp_width, bool clamp_narrow, unsigned *linux_width, unsigned *rtos_width);
int SwimLaneLayout_Resolve            (const SwimLane_Config_t *config, SwimLane_Layout_t *layout);

/*********************************************************************
*
*       Text functions
*
**********************************************************************
*/

unsigned SwimLaneText_AnsiEscapeLength(const char *str);
unsigned SwimLaneText_Utf8BytesForWidth(const char *str, unsigned max_bytes, unsigned max_cols, unsigned *out_cols);
void     SwimLaneText_WriteBytes       (FILE *stream, const char *src, unsigned src_bytes, bool color_enabled);

/*********************************************************************
*
*       Cell functions
*
**********************************************************************
*/

unsigned SwimLaneCell_Print         (FILE *stream, const char *content, unsigned width, const char *color, unsigned offset, bool color_enabled);
void     SwimLaneCell_PrintTimestamp(FILE *stream, uint64_t timestamp_us, unsigned width, bool show_time, bool color_enabled);

#if defined(__cplusplus)
}
#endif

#endif /* TRACEHUB_SWIMLANERENDERER_INTERNAL_H */

/*************************** End of file ****************************/
