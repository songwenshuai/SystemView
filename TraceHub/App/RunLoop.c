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
File    : RunLoop.c
Purpose : TraceHub mode run loop
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

#include "RunLoop.h"
#include "TraceHubSignal.h"
#include "SYS.h"
#include "Log.h"
#include "RTTBridge.h"
#include "CoreLogRecorder.h"
#include "Terminal.h"
#include "SystemView.h"
#include "LogEntry.h"
#include "LogCollector.h"
#include "LogMerger.h"
#include "SwimLaneRenderer.h"

/*********************************************************************
*
*       Static functions
*
**********************************************************************
*/

/*********************************************************************
*
*       _RunLoop_IsRunning()
*
*  Function description
*    Read the process run flag owned by the signal module.
*/
static bool _RunLoop_IsRunning(void) {
    volatile sig_atomic_t *run_flag;

    run_flag = TraceHubSignal_GetRunFlag();
    return (run_flag != NULL) && (*run_flag != 0);
}

/*********************************************************************
*
*       _SwimLaneMergerCallback()
*
*  Function description
*    Callback function for sorted swimlane output.
*/
static int _SwimLaneMergerCallback(LogEntry_t *entry, void *user_data) {
    int render_result;
    int log_result;

    (void)user_data;

    render_result = SwimLane_RenderEntry(entry);
    log_result = LogMerger_WriteEntry(entry);

    LogEntry_Destroy(entry);

    return ((render_result == 0) && (log_result == 0)) ? 0 : -1;
}

/*********************************************************************
*
*       _SwimLaneCollectorCallback()
*
*  Function description
*    Callback function for log collector output.
*    Queues entries for sorted rendering and file logging.
*/
static int _SwimLaneCollectorCallback(LogEntry_t *entry, void *user_data) {
    int result;

    (void)user_data;

    result = LogMerger_Process(entry, _SwimLaneMergerCallback, NULL);
    if (result != 0) {
        Log_Error("Swimlane merger failed to process entry: %d\n", result);
        return -1;
    }

    return 0;
}

/*********************************************************************
*
*       _StartCoreLogRecorderIfNeeded()
*
*  Function description
*    Start core log recorder for modes that consume core log channels.
*/
static int _StartCoreLogRecorderIfNeeded(const TraceHubRunLoop_Config_t *config) {
    int result;

    if (config == NULL || !config->core_log_enabled) {
        return 0;
    }

    result = CoreLogRecorder_Start();
    if (result != 0) {
        Log_Error("Failed to start core log recorder: %d\n", result);
        return -1;
    }

    return 0;
}

/*********************************************************************
*
*       _RunSwimLaneMode()
*
*  Function description
*    Run swimlane mode: dual-channel log collector and renderer.
*
*  Parameters
*    config  Pointer to runtime configuration
*
*  Return value
*    0   Normal exit
*   -1   Initialization failed
*/
static int _RunSwimLaneMode(const TraceHubRunLoop_Config_t *config) {
    int                   result;
    int                   run_result;
    int                   flush_result;
    LogCollector_Config_t collector_config;
    LogMerger_Config_t    merger_config;
    SwimLane_Config_t     swimlane_config;

    run_result = 0;

    memset(&collector_config, 0, sizeof(collector_config));
    collector_config.linux_channel    = config->linux_channel;
    collector_config.rtos_channel     = config->rtos_channel;
    collector_config.poll_interval_ms = RTT_BRIDGE_DEFAULT_POLL_INTERVAL_MS;

    result = LogCollector_Init(&collector_config);
    if (result != 0) {
        Log_Error("Failed to initialize log collector: %d\n", result);
        return -1;
    }

    memset(&merger_config, 0, sizeof(merger_config));
    merger_config.buffer_size      = LOG_MERGER_DEFAULT_BUFFER_SIZE;
    merger_config.required_source[LOG_SOURCE_LINUX] = true;
    merger_config.required_source[LOG_SOURCE_RTOS]  = true;
    merger_config.log_enabled      = true;
    merger_config.log_prefix       = config->swimlane_log_prefix;

    result = LogMerger_Init(&merger_config);
    if (result != 0) {
        Log_Error("Failed to initialize log merger: %d\n", result);
        goto err_collector;
    }

    memset(&swimlane_config, 0, sizeof(swimlane_config));
    swimlane_config.show_header    = true;
    swimlane_config.show_separator = true;
    swimlane_config.color_enabled  = true;
    swimlane_config.output_stream  = stdout;
    swimlane_config.total_width    = config->swimlane_width;
    result = SwimLane_Init(&swimlane_config);
    if (result != 0) {
        Log_Error("Failed to initialize swimlane renderer: %d\n", result);
        goto err_merger;
    }

    result = LogCollector_Start(_SwimLaneCollectorCallback, NULL);
    if (result != 0) {
        Log_Error("Failed to start log collector: %d\n", result);
        goto err_swimlane;
    }

    result = _StartCoreLogRecorderIfNeeded(config);
    if (result != 0) {
        goto err_collector_start;
    }

    if (config->sysview_enabled) {
        result = SystemView_Start();
        if (result != 0) {
            Log_Error("Failed to start SystemView service: %d\n", result);
            goto err_collector_start;
        }
    }

    printf("*                                                                    *\r\n");
    printf("*                  Swimlane Mode Started                             *\r\n");
    printf("*                                                                    *\r\n");
    printf("*  %-24.24s channel %-5u                          *\r\n",
           SWIMLANE_DEFAULT_LINUX_LABEL, config->linux_channel);
    printf("*  %-24.24s channel %-5u                          *\r\n",
           SWIMLANE_DEFAULT_RTOS_LABEL, config->rtos_channel);
    if (config->sysview_enabled) {
        if (config->systemview_network_enabled) {
            printf("*  SystemView: port %5u, channel %u                                 *\r\n",
                   config->sysview_port, config->sysview_channel);
        } else {
            printf("*  SystemView: local recording, channel %u                          *\r\n",
                   config->sysview_channel);
        }
    }
    printf("*                                                                    *\r\n");
    printf("*                  Press Ctrl+C to stop                              *\r\n");
    printf("*                                                                    *\r\n");
    printf("*********************************************************************/\r\n");

    while (_RunLoop_IsRunning()) {
        if (config->core_log_enabled && CoreLogRecorder_HasFatalError()) {
            Log_Error("Core log recorder entered fatal state\n");
            run_result = -1;
            break;
        }
        if (LogCollector_HasFatalError()) {
            Log_Error("Log collector entered fatal state\n");
            run_result = -1;
            break;
        }
        if (!LogCollector_IsRunning()) {
            Log_Error("Log collector stopped unexpectedly\n");
            run_result = -1;
            break;
        }
        if (config->sysview_enabled && SystemView_HasFatalError()) {
            Log_Error("SystemView service entered fatal state\n");
            run_result = -1;
            break;
        }
        SYS_Sleep((LOG_MERGER_DEFAULT_FLUSH_TIMEOUT_MS > 0u) ?
                  LOG_MERGER_DEFAULT_FLUSH_TIMEOUT_MS :
                  RTT_BRIDGE_DEFAULT_POLL_INTERVAL_MS);

        if (LogMerger_FlushReady(_SwimLaneMergerCallback, NULL) < 0) {
            run_result = -1;
            break;
        }
    }

    Log_Print("Shutting down swimlane services...\n");
    LogCollector_Stop();
    if (LogCollector_HasFatalError()) {
        run_result = -1;
    }
    flush_result = LogMerger_Flush(_SwimLaneMergerCallback, NULL);
    if (flush_result < 0) {
        run_result = -1;
    }
    SwimLane_Cleanup();
    LogMerger_Cleanup();
    if (LogMerger_HasFileError()) {
        run_result = -1;
    }
    LogCollector_Cleanup();
    return run_result;

err_collector_start:
    LogCollector_Stop();
err_swimlane:
    SwimLane_Cleanup();
err_merger:
    LogMerger_Cleanup();
err_collector:
    LogCollector_Cleanup();
    return -1;
}

/*********************************************************************
*
*       _RunNormalMode()
*
*  Function description
*    Run normal mode: Terminal and/or SystemView services.
*
*  Parameters
*    config  Pointer to runtime configuration
*
*  Return value
*    0   Normal exit
*   -1   Initialization failed
*/
static int _RunNormalMode(const TraceHubRunLoop_Config_t *config) {
    int   result;
    FILE *status_stream;

    result = Terminal_Start();
    if (result != 0 && config->terminal_enabled) {
        Log_Error("Failed to start Terminal service: %d\n", result);
        return -1;
    }

    result = _StartCoreLogRecorderIfNeeded(config);
    if (result != 0) {
        return -1;
    }

    result = SystemView_Start();
    if (result != 0 && config->sysview_enabled) {
        Log_Error("Failed to start SystemView service: %d\n", result);
        return -1;
    }

    status_stream = config->console_mode ? stderr : stdout;

    fprintf(status_stream, "*                                                                    *\r\n");
    if (config->only_systemview) {
        fprintf(status_stream, "*                  SystemView Recording Mode Started                 *\r\n");
        fprintf(status_stream, "*                                                                    *\r\n");
        if (config->systemview_network_enabled) {
            fprintf(status_stream, "*  SystemView: port %5u, channel %u                                 *\r\n",
                    config->sysview_port, config->sysview_channel);
        } else {
            fprintf(status_stream, "*  SystemView: local recording, channel %u                          *\r\n",
                    config->sysview_channel);
        }
        fprintf(status_stream, "*                                                                    *\r\n");
        fprintf(status_stream, "*                  Press Ctrl+C to stop                              *\r\n");
    } else if (config->console_mode) {
        fprintf(status_stream, "*                  RTT Console Mode Started                          *\r\n");
        fprintf(status_stream, "*                                                                    *\r\n");
        fprintf(status_stream, "*  Terminal:   console (stdin/stdout), channel %u                    *\r\n",
                config->terminal_channel);
        if (config->sysview_enabled) {
            if (config->systemview_network_enabled) {
                fprintf(status_stream, "*  SystemView: port %5u, channel %u                                 *\r\n",
                        config->sysview_port, config->sysview_channel);
            } else {
                fprintf(status_stream, "*  SystemView: local recording, channel %u                          *\r\n",
                        config->sysview_channel);
            }
        }
        fprintf(status_stream, "*                                                                    *\r\n");
        fprintf(status_stream, "*                  Press Ctrl+C to exit                              *\r\n");
    } else if (config->terminal_network_enabled) {
        fprintf(status_stream, "*                  RTT Telnet Mode Started                           *\r\n");
        fprintf(status_stream, "*                                                                    *\r\n");
        fprintf(status_stream, "*  Terminal:   port %5u, channel %u                                 *\r\n",
                config->terminal_port, config->terminal_channel);
        if (config->sysview_enabled) {
            if (config->systemview_network_enabled) {
                fprintf(status_stream, "*  SystemView: port %5u, channel %u                                 *\r\n",
                        config->sysview_port, config->sysview_channel);
            } else {
                fprintf(status_stream, "*  SystemView: local recording, channel %u                          *\r\n",
                        config->sysview_channel);
            }
        }
        fprintf(status_stream, "*                                                                    *\r\n");
        fprintf(status_stream, "*                  Press Ctrl+C to stop                              *\r\n");
    } else {
        fprintf(status_stream, "*                  RTT Bridge Services Started                       *\r\n");
        fprintf(status_stream, "*                                                                    *\r\n");
        if (config->terminal_enabled) {
            fprintf(status_stream, "*  Terminal:   console, channel %u                                  *\r\n",
                    config->terminal_channel);
        }
        if (config->sysview_enabled) {
            if (config->systemview_network_enabled) {
                fprintf(status_stream, "*  SystemView: port %5u, channel %u                                 *\r\n",
                        config->sysview_port, config->sysview_channel);
            } else {
                fprintf(status_stream, "*  SystemView: local recording, channel %u                          *\r\n",
                        config->sysview_channel);
            }
        }
        fprintf(status_stream, "*                                                                    *\r\n");
        fprintf(status_stream, "*                  Press Ctrl+C to stop                              *\r\n");
    }
    fprintf(status_stream, "*                                                                    *\r\n");
    fprintf(status_stream, "*********************************************************************/\r\n");
    fflush(status_stream);

    while (_RunLoop_IsRunning()) {
        if (config->core_log_enabled && CoreLogRecorder_HasFatalError()) {
            Log_Error("Core log recorder entered fatal state\n");
            return -1;
        }
        if (config->terminal_enabled && Terminal_HasFatalError()) {
            Log_Error("Terminal service entered fatal state\n");
            return -1;
        }
        if (config->sysview_enabled && SystemView_HasFatalError()) {
            Log_Error("SystemView service entered fatal state\n");
            return -1;
        }
        SYS_Sleep(100);
    }

    Log_Print("Shutting down services...\n");
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
*       TraceHubRunLoop_Run()
*
*  Function description
*    Run the selected TraceHub service mode.
*
*  Parameters
*    config  Pointer to runtime configuration
*
*  Return value
*    0   Normal exit
*   -1   Invalid configuration or runtime failure
*/
int TraceHubRunLoop_Run(const TraceHubRunLoop_Config_t *config) {
    if (config == NULL) {
        return -1;
    }

    if (config->swimlane_mode) {
        return _RunSwimLaneMode(config);
    }
    return _RunNormalMode(config);
}

/*************************** End of file ****************************/
