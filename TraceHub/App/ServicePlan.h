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
File    : ServicePlan.h
Purpose : TraceHub service plan construction
---------------------------END-OF-HEADER------------------------------
*/

#ifndef TRACEHUB_SERVICEPLAN_H
#define TRACEHUB_SERVICEPLAN_H

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include <stdbool.h>
#include <signal.h>

#include "Options.h"
#include "RunLoop.h"
#include "RTTBridge.h"
#include "CoreLogRecorder.h"
#include "Terminal.h"
#include "SystemView.h"

#if defined(__cplusplus)
extern "C" {
#endif

/*********************************************************************
*
*       Defines
*
**********************************************************************
*/

#ifndef TRACEHUB_SERVICE_PLAN_PATH_SIZE
  #define TRACEHUB_SERVICE_PLAN_PATH_SIZE 4096
#endif

/*********************************************************************
*
*       Types
*
**********************************************************************
*/

/*********************************************************************
*
*       TraceHubServicePlan_t
*
*  Description
*    Fully resolved service initialization and run-loop plan.
*/
typedef struct {
    bool                      core_log_enabled;
    bool                      terminal_service_enabled;
    bool                      sysview_enabled;
    bool                      swimlane_mode;

    RTTBridge_Config_t        bridge_config;
    CoreLogRecorder_Config_t  core_log_config;
    Terminal_Config_t         terminal_config;
    SystemView_Config_t       sysview_config;
    TraceHubRunLoop_Config_t  run_loop_config;

    char                      main_log_prefix[TRACEHUB_SERVICE_PLAN_PATH_SIZE];
    char                      linux_log_prefix[TRACEHUB_SERVICE_PLAN_PATH_SIZE];
    char                      rtos_log_prefix[TRACEHUB_SERVICE_PLAN_PATH_SIZE];
    char                      swimlane_log_prefix[TRACEHUB_SERVICE_PLAN_PATH_SIZE];
    char                      sysview_log_prefix[TRACEHUB_SERVICE_PLAN_PATH_SIZE];
} TraceHubServicePlan_t;

/*********************************************************************
*
*       API functions
*
**********************************************************************
*/

int TraceHubServicePlan_Build(const TraceHubOptions_t *options,
                              volatile sig_atomic_t *run_flag,
                              TraceHubServicePlan_t *plan);

#if defined(__cplusplus)
}
#endif

#endif /* TRACEHUB_SERVICEPLAN_H */

/*************************** End of file ****************************/
