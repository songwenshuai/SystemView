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
File    : ServicePlan.c
Purpose : TraceHub service plan construction
---------------------------END-OF-HEADER------------------------------
*/

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include <errno.h>
#include <stdio.h>
#include <string.h>

#if defined(_WIN32)
  #include <sys/stat.h>
  #define stat _stat
  #ifndef S_ISDIR
    #define S_ISDIR(mode) (((mode) & _S_IFDIR) != 0)
  #endif
#else
  #include <sys/stat.h>
#endif

#include "ServicePlan.h"
#include "TraceHubDefaults.h"

/*********************************************************************
*
*       Static functions
*
**********************************************************************
*/

/*********************************************************************
*
*       _BuildLogPrefix()
*
*  Function description
*    Build a file prefix for timestamped log creation.
*
*  Return value
*    0   Success
*   -1   Invalid input or output buffer too small
*/
static int _BuildLogPrefix(char *buffer, size_t buffer_size, const char *log_dir, const char *name) {
    size_t dir_len;
    int    length;

    if ((buffer == NULL) || (buffer_size == 0u) || (name == NULL) || (name[0] == '\0')) {
        return -1;
    }

    if ((log_dir == NULL) || (log_dir[0] == '\0')) {
        length = snprintf(buffer, buffer_size, "%s", name);
    } else {
        dir_len = strlen(log_dir);
        if ((log_dir[dir_len - 1u] == '/') || (log_dir[dir_len - 1u] == '\\')) {
            length = snprintf(buffer, buffer_size, "%s%s", log_dir, name);
        } else {
            length = snprintf(buffer, buffer_size, "%s/%s", log_dir, name);
        }
    }

    if ((length < 0) || ((size_t)length >= buffer_size)) {
        return -1;
    }
    return 0;
}

/*********************************************************************
*
*       _ValidateLogDir()
*
*  Function description
*    Validate user supplied log directory before any log file is created.
*
*  Return value
*    0   Success or no explicit directory supplied
*   -1   Directory is missing, inaccessible, or not a directory
*/
static int _ValidateLogDir(const char *log_dir) {
    struct stat st;

    if ((log_dir == NULL) || (log_dir[0] == '\0')) {
        return 0;
    }

    if (stat(log_dir, &st) != 0) {
        fprintf(stderr, "Error: --log-dir '%s' is not accessible: %s\n",
                log_dir, strerror(errno));
        return -1;
    }
    if (!S_ISDIR(st.st_mode)) {
        fprintf(stderr, "Error: --log-dir '%s' is not a directory.\n", log_dir);
        return -1;
    }

    return 0;
}

/*********************************************************************
*
*       _EnableCoreLogForTerminalChannel()
*
*  Function description
*    Enable core log recording when the Terminal default channel matches
*    one of the named core log sources.
*/
static void _EnableCoreLogForTerminalChannel(unsigned terminal_channel,
                                             unsigned linux_channel,
                                             unsigned rtos_channel,
                                             bool *linux_enabled,
                                             bool *rtos_enabled) {
    if ((linux_enabled == NULL) || (rtos_enabled == NULL)) {
        return;
    }

    if (terminal_channel == linux_channel) {
        *linux_enabled = true;
    } else if (terminal_channel == rtos_channel) {
        *rtos_enabled = true;
    }
}

/*********************************************************************
*
*       Public functions
*
**********************************************************************
*/

/*********************************************************************
*
*       TraceHubServicePlan_Build()
*
*  Function description
*    Resolve user options into a complete service initialization plan.
*
*  Parameters
*    options   Parsed raw command-line options
*    run_flag  Shared process run flag
*    plan      Output service plan
*
*  Return value
*    0   Success
*   -1   Invalid option combination or plan construction failure
*/
int TraceHubServicePlan_Build(const TraceHubOptions_t *options,
                              volatile sig_atomic_t *run_flag,
                              TraceHubServicePlan_t *plan) {
    unsigned    terminal_port;
    unsigned    sysview_port;
    unsigned    terminal_channel;
    unsigned    sysview_channel;
    unsigned    linux_channel;
    unsigned    rtos_channel;
    unsigned    rtt_search_timeout_ms;
    unsigned    swimlane_width;
    uint64_t    rtt_address;
    size_t      rtt_region_size;
    const char *device_path;
    const char *log_dir;
    bool        terminal_enabled;
    bool        sysview_enabled;
    bool        console_mode;
    bool        swimlane_mode;
    bool        only_systemview;
    bool        terminal_network_enabled;
    bool        systemview_network_enabled;
    bool        linux_source_requested;
    bool        rtos_source_requested;
    bool        core_log_linux_enabled;
    bool        core_log_rtos_enabled;
    bool        core_log_enabled;
    bool        terminal_service_enabled;

    if ((options == NULL) || (run_flag == NULL) || (plan == NULL)) {
        return -1;
    }

    memset(plan, 0, sizeof(*plan));

    terminal_port = options->terminal_port_specified ?
                    options->terminal_port : RTT_BRIDGE_DEFAULT_TERMINAL_PORT;
    sysview_port = options->systemview_port_specified ?
                   options->systemview_port : RTT_BRIDGE_DEFAULT_SYSVIEW_PORT;
    terminal_channel = RTT_BRIDGE_DEFAULT_TERMINAL_CHANNEL;
    sysview_channel = options->systemview_channel_specified ?
                      options->systemview_channel : RTT_BRIDGE_DEFAULT_SYSVIEW_CHANNEL;
    linux_channel = options->linux_channel_specified ?
                    options->linux_channel : RTT_BRIDGE_DEFAULT_LINUX_CHANNEL;
    rtos_channel = options->rtos_channel_specified ?
                   options->rtos_channel : RTT_BRIDGE_DEFAULT_RTOS_CHANNEL;
    rtt_search_timeout_ms = options->rtt_search_timeout_specified ?
                            options->rtt_search_timeout_ms : 0u;
    swimlane_width = options->swimlane_width_specified ?
                     options->swimlane_width : 0u;
    rtt_address = options->rtt_address_specified ?
                  options->rtt_address : TRACEHUB_DEFAULT_RTT_ADDR;
    rtt_region_size = options->rtt_region_size_specified ?
                      options->rtt_region_size : TRACEHUB_DEFAULT_RTT_SIZE;
    device_path = options->device_path_specified ?
                  options->device_path : TRACEHUB_DEFAULT_MEMORY_PATH;
    log_dir = options->log_dir_specified ? options->log_dir : NULL;

    terminal_enabled = true;
    console_mode = options->console_requested;
    swimlane_mode = options->swimlane_requested;
    terminal_network_enabled = options->terminal_port_specified;
    systemview_network_enabled = options->systemview_port_specified;
    linux_source_requested = false;
    rtos_source_requested = false;
    core_log_linux_enabled = false;
    core_log_rtos_enabled = false;

    if (console_mode && swimlane_mode) {
        printf("Error: --console and --swimlane are mutually exclusive.\n");
        return -1;
    }
    if (console_mode && terminal_network_enabled) {
        printf("Error: --console and --telnet-port are mutually exclusive.\n");
        return -1;
    }
    if (swimlane_mode && terminal_network_enabled) {
        printf("Error: --swimlane and --telnet-port are mutually exclusive.\n");
        return -1;
    }
    if (options->linux_requested && options->rtos_requested) {
        printf("Error: --linux and --rtos are mutually exclusive.\n");
        return -1;
    }

    sysview_enabled = options->systemview_requested ||
                      options->systemview_port_specified ||
                      options->systemview_channel_specified;

    only_systemview = sysview_enabled &&
                      !console_mode &&
                      !swimlane_mode &&
                      !terminal_network_enabled &&
                      !options->linux_requested &&
                      !options->rtos_requested &&
                      !options->linux_channel_specified &&
                      !options->rtos_channel_specified;

    if (!only_systemview &&
        !console_mode &&
        !swimlane_mode &&
        !terminal_network_enabled) {
        if (options->linux_requested || options->rtos_requested) {
            console_mode = true;
        } else {
            swimlane_mode = true;
        }
    }

    if (swimlane_mode && (options->linux_requested || options->rtos_requested)) {
        printf("Error: --linux and --rtos select a single-source mode and cannot be used with --swimlane.\n");
        return -1;
    }

    if (console_mode || terminal_network_enabled) {
        linux_source_requested = options->linux_requested || options->linux_channel_specified;
        rtos_source_requested  = options->rtos_requested || options->rtos_channel_specified;
        if (linux_source_requested && rtos_source_requested) {
            printf("Error: Linux and RTOS source options are mutually exclusive in single-source modes.\n");
            return -1;
        }
    }

    if (options->swimlane_width_specified && !swimlane_mode) {
        printf("Error: --swimlane-width requires swimlane mode.\n");
        return -1;
    }

    if (only_systemview) {
        terminal_enabled = false;
    } else if (terminal_network_enabled) {
        terminal_enabled = true;
        console_mode = false;
        swimlane_mode = false;

        if (rtos_source_requested) {
            terminal_channel = rtos_channel;
            core_log_rtos_enabled = true;
        } else if (linux_source_requested) {
            terminal_channel = linux_channel;
            core_log_linux_enabled = true;
        } else {
            _EnableCoreLogForTerminalChannel(terminal_channel,
                                             linux_channel,
                                             rtos_channel,
                                             &core_log_linux_enabled,
                                             &core_log_rtos_enabled);
        }
    } else if (swimlane_mode) {
        terminal_enabled = true;
        core_log_linux_enabled = true;
        core_log_rtos_enabled = true;
    } else {
        terminal_enabled = true;
        console_mode = true;
        swimlane_mode = false;

        if (rtos_source_requested) {
            terminal_channel = rtos_channel;
            core_log_rtos_enabled = true;
        } else if (linux_source_requested) {
            terminal_channel = linux_channel;
            core_log_linux_enabled = true;
        } else {
            _EnableCoreLogForTerminalChannel(terminal_channel,
                                             linux_channel,
                                             rtos_channel,
                                             &core_log_linux_enabled,
                                             &core_log_rtos_enabled);
        }
    }

    core_log_enabled = core_log_linux_enabled || core_log_rtos_enabled;
    terminal_service_enabled = terminal_enabled && !swimlane_mode;

    if (!terminal_enabled && !sysview_enabled) {
        printf("Error: No service enabled.\n");
        return -1;
    }

    if (core_log_linux_enabled && core_log_rtos_enabled && linux_channel == rtos_channel) {
        printf("Error: Linux channel (%u) conflicts with RTOS channel (%u).\n",
               linux_channel, rtos_channel);
        printf("Core log recording requires two different RTT channels.\n");
        return -1;
    }

    if (sysview_enabled && core_log_enabled) {
        if (core_log_linux_enabled && sysview_channel == linux_channel) {
            printf("Error: SystemView channel (%u) conflicts with Linux log channel (%u).\n",
                   sysview_channel, linux_channel);
            printf("Each enabled service must use a different RTT channel.\n");
            return -1;
        }
        if (core_log_rtos_enabled && sysview_channel == rtos_channel) {
            printf("Error: SystemView channel (%u) conflicts with RTOS log channel (%u).\n",
                   sysview_channel, rtos_channel);
            printf("Each enabled service must use a different RTT channel.\n");
            return -1;
        }
        if (!swimlane_mode && terminal_enabled && sysview_channel == terminal_channel) {
            printf("Error: SystemView channel (%u) conflicts with Terminal channel (%u).\n",
                   sysview_channel, terminal_channel);
            printf("Each service must use a different RTT channel.\n");
            return -1;
        }
    } else if (sysview_enabled && terminal_enabled && sysview_channel == terminal_channel) {
        printf("Error: SystemView channel (%u) conflicts with Terminal channel (%u).\n",
               sysview_channel, terminal_channel);
        printf("Each service must use a different RTT channel.\n");
        return -1;
    }

    if (_ValidateLogDir(log_dir) != 0) {
        return -1;
    }

    if ((_BuildLogPrefix(plan->main_log_prefix, sizeof(plan->main_log_prefix), log_dir, "main") != 0) ||
        (_BuildLogPrefix(plan->linux_log_prefix, sizeof(plan->linux_log_prefix), log_dir, "linux") != 0) ||
        (_BuildLogPrefix(plan->rtos_log_prefix, sizeof(plan->rtos_log_prefix), log_dir, "rtos") != 0) ||
        (_BuildLogPrefix(plan->swimlane_log_prefix, sizeof(plan->swimlane_log_prefix), log_dir, "swimlane") != 0) ||
        (_BuildLogPrefix(plan->sysview_log_prefix, sizeof(plan->sysview_log_prefix), log_dir, "sysview") != 0)) {
        printf("Error: log path prefix is too long.\n");
        return -1;
    }

    plan->core_log_enabled = core_log_enabled;
    plan->terminal_service_enabled = terminal_service_enabled;
    plan->sysview_enabled = sysview_enabled;
    plan->swimlane_mode = swimlane_mode;

    plan->bridge_config.rtt_address = rtt_address;
    plan->bridge_config.rtt_region_size = rtt_region_size;
    plan->bridge_config.poll_interval_ms = RTT_BRIDGE_DEFAULT_POLL_INTERVAL_MS;
    plan->bridge_config.device_path = device_path;
    plan->bridge_config.run_flag = run_flag;
    plan->bridge_config.rtt_search_timeout_ms = rtt_search_timeout_ms;
    plan->bridge_config.reset_memory = options->memshm_reset_requested;
    plan->bridge_config.debug = false;

    plan->core_log_config.linux_channel = linux_channel;
    plan->core_log_config.rtos_channel = rtos_channel;
    plan->core_log_config.poll_interval_ms = RTT_BRIDGE_DEFAULT_POLL_INTERVAL_MS;
    plan->core_log_config.linux_prefix = plan->linux_log_prefix;
    plan->core_log_config.rtos_prefix = plan->rtos_log_prefix;
    plan->core_log_config.linux_enabled = core_log_linux_enabled;
    plan->core_log_config.rtos_enabled = core_log_rtos_enabled;

    plan->terminal_config.port = terminal_port;
    plan->terminal_config.channel = terminal_channel;
    plan->terminal_config.enabled = terminal_service_enabled;
    plan->terminal_config.console_mode = console_mode;

    plan->sysview_config.port = sysview_port;
    plan->sysview_config.channel = sysview_channel;
    plan->sysview_config.enabled = sysview_enabled;
    plan->sysview_config.network_enabled = systemview_network_enabled;
    plan->sysview_config.record_enabled = sysview_enabled;
    plan->sysview_config.record_prefix = plan->sysview_log_prefix;

    plan->run_loop_config.linux_channel = linux_channel;
    plan->run_loop_config.rtos_channel = rtos_channel;
    plan->run_loop_config.terminal_channel = terminal_channel;
    plan->run_loop_config.sysview_channel = sysview_channel;
    plan->run_loop_config.terminal_port = terminal_port;
    plan->run_loop_config.sysview_port = sysview_port;
    plan->run_loop_config.swimlane_mode = swimlane_mode;
    plan->run_loop_config.terminal_enabled = terminal_service_enabled;
    plan->run_loop_config.sysview_enabled = sysview_enabled;
    plan->run_loop_config.core_log_enabled = core_log_enabled;
    plan->run_loop_config.console_mode = console_mode;
    plan->run_loop_config.only_systemview = only_systemview;
    plan->run_loop_config.terminal_network_enabled = terminal_network_enabled;
    plan->run_loop_config.systemview_network_enabled = systemview_network_enabled;
    plan->run_loop_config.swimlane_width = swimlane_width;
    plan->run_loop_config.swimlane_log_prefix = plan->swimlane_log_prefix;

    return 0;
}

/*************************** End of file ****************************/
