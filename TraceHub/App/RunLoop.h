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
File    : RunLoop.h
Purpose : TraceHub mode run loop
---------------------------END-OF-HEADER------------------------------
*/

#ifndef TRACEHUB_RUNLOOP_H
#define TRACEHUB_RUNLOOP_H

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include <stdbool.h>

#if defined(__cplusplus)
extern "C" {
#endif

/*********************************************************************
*
*       Types
*
**********************************************************************
*/

/*********************************************************************
*
*       TraceHubRunLoop_Config_t
*
*  Description
*    Runtime configuration for mode-specific run loops.
*/
typedef struct {
    unsigned    linux_channel;
    unsigned    rtos_channel;
    unsigned    terminal_channel;
    unsigned    sysview_channel;
    unsigned    terminal_port;
    unsigned    sysview_port;
    bool        swimlane_mode;
    bool        terminal_enabled;
    bool        sysview_enabled;
    bool        core_log_enabled;
    bool        console_mode;
    bool        only_systemview;
    bool        terminal_network_enabled;
    bool        systemview_network_enabled;
    unsigned    swimlane_width;
    const char *swimlane_log_prefix;
} TraceHubRunLoop_Config_t;

/*********************************************************************
*
*       API functions
*
**********************************************************************
*/

int TraceHubRunLoop_Run(const TraceHubRunLoop_Config_t *config);

#if defined(__cplusplus)
}
#endif

#endif /* TRACEHUB_RUNLOOP_H */

/*************************** End of file ****************************/
