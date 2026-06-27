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
File    : main.c
Purpose : TraceHub application entry point
---------------------------END-OF-HEADER------------------------------
*/

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include <locale.h>
#include <stdbool.h>
#include <stdio.h>

#include "CLI.h"
#include "Options.h"
#include "ServicePlan.h"
#include "TraceHubSignal.h"
#include "RunLoop.h"
#include "Log.h"
#include "RTTBridge.h"
#include "CoreLogRecorder.h"
#include "Terminal.h"
#include "SystemView.h"

/*********************************************************************
*
*       Defines, fixed
*
**********************************************************************
*/

#ifndef TRACEHUB_VERSION
  #define TRACEHUB_VERSION "0.0.0"
#endif

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

static const char _TRACEHUB[] = "TraceHub";

/*********************************************************************
*
*       Public code
*
**********************************************************************
*/

/*********************************************************************
*
*       main()
*
*/
int main(int argc, char *argv[]) {
    TraceHubOptions_t     options;
    TraceHubServicePlan_t plan;
    bool                  core_log_fatal;
    bool                  terminal_fatal;
    bool                  sysview_fatal;
    int                   result;

    setlocale(LC_CTYPE, "");

    if (TraceHubOptions_Parse(argc, argv, &options) != 0) {
        return 1;
    }
    if (options.help_requested) {
        CLI_PrintUsage();
        return 0;
    }
    if (options.version_requested) {
        printf("%s %s\n", _TRACEHUB, TRACEHUB_VERSION);
        return 0;
    }

    if (TraceHubServicePlan_Build(&options, TraceHubSignal_GetRunFlag(), &plan) != 0) {
        return 1;
    }

    CLI_PrintBanner(plan.run_loop_config.console_mode ? stderr : stdout);

    result = LOG_InitEx(plan.main_log_prefix);
    if (result != 0) {
        fprintf(stderr, "Warning: Failed to initialize main log file: %d\n", result);
    }

    TraceHubSignal_Init();

    result = RTTBridge_Init(&plan.bridge_config);
    if (result != 0) {
        Log_Error("Failed to initialize RTT bridge: %d\n", result);
        goto err_rttbridge;
    }

    if (plan.core_log_enabled) {
        result = CoreLogRecorder_Init(&plan.core_log_config);
        if (result != 0) {
            Log_Error("Failed to initialize core log recorder: %d\n", result);
            goto err_corelog;
        }
    }

    result = Terminal_Init(&plan.terminal_config);
    if (result != 0) {
        Log_Error("Failed to initialize Terminal service: %d\n", result);
        goto err_terminal;
    }

    result = SystemView_Init(&plan.sysview_config);
    if (result != 0) {
        Log_Error("Failed to initialize SystemView service: %d\n", result);
        goto err_sysview;
    }

    RTTBridge_SetRunning(true);

    result = TraceHubRunLoop_Run(&plan.run_loop_config);
    if (result != 0) {
        goto err_sysview;
    }

    SystemView_Stop();
    Terminal_Stop();
    terminal_fatal = Terminal_HasFatalError();
    if (plan.core_log_enabled) {
        CoreLogRecorder_Stop();
        core_log_fatal = CoreLogRecorder_HasFatalError();
        CoreLogRecorder_Cleanup();
    } else {
        core_log_fatal = false;
    }
    sysview_fatal = SystemView_HasFatalError();
    RTTBridge_Cleanup();
    Log_Print("tracehub exit.\n");
    LOG_Cleanup();
    return (core_log_fatal || terminal_fatal || sysview_fatal) ? 1 : 0;

err_sysview:
    SystemView_Stop();
err_terminal:
    Terminal_Stop();
    if (plan.core_log_enabled) {
        CoreLogRecorder_Stop();
        CoreLogRecorder_Cleanup();
    }
err_corelog:
err_rttbridge:
    RTTBridge_Cleanup();
    LOG_Cleanup();
    return 1;
}

/*************************** End of file ****************************/
