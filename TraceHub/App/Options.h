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
File    : Options.h
Purpose : TraceHub command line option parsing
---------------------------END-OF-HEADER------------------------------
*/

#ifndef TRACEHUB_OPTIONS_H
#define TRACEHUB_OPTIONS_H

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

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
*       TraceHubOptions_t
*
*  Description
*    Raw user intent parsed from command line tokens.
*/
typedef struct {
    bool        help_requested;
    bool        version_requested;
    bool        console_requested;
    bool        swimlane_requested;
    bool        systemview_requested;
    bool        linux_requested;
    bool        rtos_requested;
    bool        memshm_reset_requested;

    bool        rtt_address_specified;
    uint64_t    rtt_address;

    bool        rtt_region_size_specified;
    size_t      rtt_region_size;

    bool        device_path_specified;
    const char *device_path;

    bool        terminal_port_specified;
    unsigned    terminal_port;

    bool        systemview_port_specified;
    unsigned    systemview_port;

    bool        linux_channel_specified;
    unsigned    linux_channel;

    bool        rtos_channel_specified;
    unsigned    rtos_channel;

    bool        systemview_channel_specified;
    unsigned    systemview_channel;

    bool        rtt_search_timeout_specified;
    unsigned    rtt_search_timeout_ms;

    bool        log_dir_specified;
    const char *log_dir;

    bool        swimlane_width_specified;
    unsigned    swimlane_width;
} TraceHubOptions_t;

/*********************************************************************
*
*       API functions
*
**********************************************************************
*/

int TraceHubOptions_Parse(int argc, char *argv[], TraceHubOptions_t *options);

#if defined(__cplusplus)
}
#endif

#endif /* TRACEHUB_OPTIONS_H */

/*************************** End of file ****************************/
