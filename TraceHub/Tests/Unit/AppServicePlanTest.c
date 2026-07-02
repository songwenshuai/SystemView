/*********************************************************************
*                             CineLogic                              *
*                      RTT Trace and Debug Bridge                    *
**********************************************************************
----------------------------------------------------------------------
File    : AppServicePlanTest.c
Purpose : Unit checks for application service plan construction
---------------------------END-OF-HEADER------------------------------
*/

#include <stdio.h>
#include <string.h>

#include "ServicePlan.h"
#include "TestCommon.h"

static int _TestDefaultServicePlanUsesSwimlane(void) {
    TraceHubOptions_t      options;
    TraceHubServicePlan_t  plan;
    volatile sig_atomic_t  run_flag;

    memset(&options, 0, sizeof(options));
    run_flag = 1;
    TEST_ASSERT(TraceHubServicePlan_Build(&options, &run_flag, &plan) == 0);
    TEST_ASSERT(plan.swimlane_mode);
    TEST_ASSERT(plan.core_log_enabled);
    TEST_ASSERT(!plan.terminal_service_enabled);
    TEST_ASSERT(!plan.sysview_enabled);
    TEST_ASSERT(plan.core_log_config.linux_enabled);
    TEST_ASSERT(plan.core_log_config.rtos_enabled);
    TEST_ASSERT(plan.bridge_config.run_flag == &run_flag);
    TEST_ASSERT(plan.terminal_config.enabled == false);
    TEST_ASSERT(plan.run_loop_config.swimlane_mode);
    TEST_ASSERT(strcmp(plan.main_log_prefix, "main") == 0);
    TEST_ASSERT(strcmp(plan.linux_log_prefix, "linux") == 0);
    TEST_ASSERT(strcmp(plan.rtos_log_prefix, "rtos") == 0);
    return 0;
}

static int _TestAdditionalServicePlanConfigurations(void) {
    TraceHubOptions_t      options;
    TraceHubServicePlan_t  plan;
    volatile sig_atomic_t  run_flag;
    FILE                  *file;
    const char            *file_path = "tracehub_unit_log_dir_file.tmp";
    int                    result;

    run_flag = 1;
    memset(&options, 0, sizeof(options));
    options.terminal_port_specified = true;
    options.terminal_port = 2323u;
    options.systemview_port_specified = true;
    options.systemview_port = 19111u;
    options.rtt_address_specified = true;
    options.rtt_address = 0x2000u;
    options.rtt_region_size_specified = true;
    options.rtt_region_size = 8192u;
    options.device_path_specified = true;
    options.device_path = "/tracehub_unit";
    options.rtt_search_timeout_specified = true;
    options.rtt_search_timeout_ms = 500u;
    options.memshm_reset_requested = true;
    TEST_ASSERT(TraceHubServicePlan_Build(&options, &run_flag, &plan) == 0);
    TEST_ASSERT(!plan.swimlane_mode);
    TEST_ASSERT(plan.terminal_service_enabled);
    TEST_ASSERT(plan.terminal_config.port == 2323u);
    TEST_ASSERT(!plan.terminal_config.console_mode);
    TEST_ASSERT(plan.core_log_enabled);
    TEST_ASSERT(plan.core_log_config.linux_enabled);
    TEST_ASSERT(!plan.core_log_config.rtos_enabled);
    TEST_ASSERT(plan.sysview_enabled);
    TEST_ASSERT(plan.sysview_config.network_enabled);
    TEST_ASSERT(plan.bridge_config.rtt_address == 0x2000u);
    TEST_ASSERT(plan.bridge_config.rtt_region_size == 8192u);
    TEST_ASSERT(strcmp(plan.bridge_config.device_path, "/tracehub_unit") == 0);
    TEST_ASSERT(plan.bridge_config.rtt_search_timeout_ms == 500u);
    TEST_ASSERT(plan.bridge_config.reset_memory);

    memset(&options, 0, sizeof(options));
    options.terminal_port_specified = true;
    options.terminal_port = 2324u;
    options.rtos_requested = true;
    options.rtos_channel_specified = true;
    options.rtos_channel = 5u;
    TEST_ASSERT(TraceHubServicePlan_Build(&options, &run_flag, &plan) == 0);
    TEST_ASSERT(plan.terminal_config.channel == 5u);
    TEST_ASSERT(!plan.core_log_config.linux_enabled);
    TEST_ASSERT(plan.core_log_config.rtos_enabled);

    memset(&options, 0, sizeof(options));
    options.log_dir_specified = true;
    options.log_dir = "./";
    TEST_ASSERT(TraceHubServicePlan_Build(&options, &run_flag, &plan) == 0);
    TEST_ASSERT(strcmp(plan.main_log_prefix, "./main") == 0);
    TEST_ASSERT(strcmp(plan.swimlane_log_prefix, "./swimlane") == 0);

    file = fopen(file_path, "w");
    TEST_ASSERT(file != NULL);
    TEST_ASSERT(fclose(file) == 0);
    memset(&options, 0, sizeof(options));
    options.log_dir_specified = true;
    options.log_dir = file_path;
    result = TraceHubServicePlan_Build(&options, &run_flag, &plan);
    TEST_ASSERT(remove(file_path) == 0);
    TEST_ASSERT(result == -1);
    return 0;
}

static int _TestSingleSourceAndSystemViewPlans(void) {
    TraceHubOptions_t      options;
    TraceHubServicePlan_t  plan;
    volatile sig_atomic_t  run_flag;

    run_flag = 1;
    memset(&options, 0, sizeof(options));
    options.console_requested = true;
    options.rtos_requested = true;
    options.rtos_channel_specified = true;
    options.rtos_channel = 4u;
    options.log_dir_specified = true;
    options.log_dir = ".";
    TEST_ASSERT(TraceHubServicePlan_Build(&options, &run_flag, &plan) == 0);
    TEST_ASSERT(!plan.swimlane_mode);
    TEST_ASSERT(plan.terminal_service_enabled);
    TEST_ASSERT(plan.terminal_config.console_mode);
    TEST_ASSERT(plan.terminal_config.channel == 4u);
    TEST_ASSERT(!plan.core_log_config.linux_enabled);
    TEST_ASSERT(plan.core_log_config.rtos_enabled);
    TEST_ASSERT(strcmp(plan.main_log_prefix, "./main") == 0);

    memset(&options, 0, sizeof(options));
    options.systemview_requested = true;
    TEST_ASSERT(TraceHubServicePlan_Build(&options, &run_flag, &plan) == 0);
    TEST_ASSERT(!plan.swimlane_mode);
    TEST_ASSERT(!plan.terminal_service_enabled);
    TEST_ASSERT(!plan.core_log_enabled);
    TEST_ASSERT(plan.sysview_enabled);
    TEST_ASSERT(plan.sysview_config.record_enabled);
    TEST_ASSERT(!plan.sysview_config.network_enabled);
    TEST_ASSERT(plan.run_loop_config.only_systemview);

    memset(&options, 0, sizeof(options));
    options.linux_requested = true;
    TEST_ASSERT(TraceHubServicePlan_Build(&options, &run_flag, &plan) == 0);
    TEST_ASSERT(!plan.swimlane_mode);
    TEST_ASSERT(plan.terminal_service_enabled);
    TEST_ASSERT(plan.terminal_config.console_mode);
    TEST_ASSERT(plan.terminal_config.channel == RTT_BRIDGE_DEFAULT_LINUX_CHANNEL);
    TEST_ASSERT(plan.core_log_config.linux_enabled);
    TEST_ASSERT(!plan.core_log_config.rtos_enabled);

    memset(&options, 0, sizeof(options));
    options.terminal_port_specified = true;
    options.terminal_port = 2325u;
    options.linux_requested = true;
    options.linux_channel_specified = true;
    options.linux_channel = 3u;
    TEST_ASSERT(TraceHubServicePlan_Build(&options, &run_flag, &plan) == 0);
    TEST_ASSERT(plan.terminal_service_enabled);
    TEST_ASSERT(!plan.terminal_config.console_mode);
    TEST_ASSERT(plan.terminal_config.channel == 3u);
    TEST_ASSERT(plan.core_log_config.linux_enabled);
    TEST_ASSERT(!plan.core_log_config.rtos_enabled);

    memset(&options, 0, sizeof(options));
    options.console_requested = true;
    TEST_ASSERT(TraceHubServicePlan_Build(&options, &run_flag, &plan) == 0);
    TEST_ASSERT(!plan.swimlane_mode);
    TEST_ASSERT(plan.terminal_service_enabled);
    TEST_ASSERT(plan.terminal_config.console_mode);
    TEST_ASSERT(plan.terminal_config.channel == RTT_BRIDGE_DEFAULT_TERMINAL_CHANNEL);
    TEST_ASSERT(plan.core_log_config.linux_enabled);
    TEST_ASSERT(!plan.core_log_config.rtos_enabled);
    return 0;
}

static int _TestServicePlanRejectsConflicts(void) {
    TraceHubOptions_t      options;
    TraceHubServicePlan_t  plan;
    volatile sig_atomic_t  run_flag;

    run_flag = 1;
    memset(&options, 0, sizeof(options));
    options.console_requested = true;
    options.swimlane_requested = true;
    TEST_ASSERT(TraceHubServicePlan_Build(&options, &run_flag, &plan) == -1);

    memset(&options, 0, sizeof(options));
    options.console_requested = true;
    options.terminal_port_specified = true;
    options.terminal_port = 2323u;
    TEST_ASSERT(TraceHubServicePlan_Build(&options, &run_flag, &plan) == -1);

    memset(&options, 0, sizeof(options));
    options.swimlane_requested = true;
    options.terminal_port_specified = true;
    options.terminal_port = 2323u;
    TEST_ASSERT(TraceHubServicePlan_Build(&options, &run_flag, &plan) == -1);

    memset(&options, 0, sizeof(options));
    options.linux_requested = true;
    options.rtos_requested = true;
    TEST_ASSERT(TraceHubServicePlan_Build(&options, &run_flag, &plan) == -1);

    memset(&options, 0, sizeof(options));
    options.swimlane_requested = true;
    options.linux_requested = true;
    TEST_ASSERT(TraceHubServicePlan_Build(&options, &run_flag, &plan) == -1);

    memset(&options, 0, sizeof(options));
    options.console_requested = true;
    options.linux_channel_specified = true;
    options.linux_channel = 3u;
    options.rtos_requested = true;
    TEST_ASSERT(TraceHubServicePlan_Build(&options, &run_flag, &plan) == -1);

    memset(&options, 0, sizeof(options));
    options.swimlane_requested = true;
    options.systemview_requested = true;
    options.systemview_channel_specified = true;
    options.systemview_channel = RTT_BRIDGE_DEFAULT_LINUX_CHANNEL;
    TEST_ASSERT(TraceHubServicePlan_Build(&options, &run_flag, &plan) == -1);

    memset(&options, 0, sizeof(options));
    options.swimlane_width_specified = true;
    options.swimlane_width = 160u;
    options.console_requested = true;
    TEST_ASSERT(TraceHubServicePlan_Build(&options, &run_flag, &plan) == -1);

    memset(&options, 0, sizeof(options));
    options.swimlane_requested = true;
    options.linux_channel_specified = true;
    options.linux_channel = 1u;
    options.rtos_channel_specified = true;
    options.rtos_channel = 1u;
    TEST_ASSERT(TraceHubServicePlan_Build(&options, &run_flag, &plan) == -1);

    memset(&options, 0, sizeof(options));
    options.terminal_port_specified = true;
    options.terminal_port = 2323u;
    options.systemview_requested = true;
    options.systemview_channel_specified = true;
    options.systemview_channel = RTT_BRIDGE_DEFAULT_TERMINAL_CHANNEL;
    TEST_ASSERT(TraceHubServicePlan_Build(&options, &run_flag, &plan) == -1);

    memset(&options, 0, sizeof(options));
    options.console_requested = true;
    options.rtos_requested = true;
    options.systemview_requested = true;
    options.systemview_channel_specified = true;
    options.systemview_channel = RTT_BRIDGE_DEFAULT_RTOS_CHANNEL;
    TEST_ASSERT(TraceHubServicePlan_Build(&options, &run_flag, &plan) == -1);

    memset(&options, 0, sizeof(options));
    options.log_dir_specified = true;
    options.log_dir = "tracehub_unit_missing_dir";
    TEST_ASSERT(TraceHubServicePlan_Build(&options, &run_flag, &plan) == -1);

    TEST_ASSERT(TraceHubServicePlan_Build(NULL, &run_flag, &plan) == -1);
    TEST_ASSERT(TraceHubServicePlan_Build(&options, NULL, &plan) == -1);
    TEST_ASSERT(TraceHubServicePlan_Build(&options, &run_flag, NULL) == -1);
    return 0;
}

int main(void) {
    TEST_RUN(_TestDefaultServicePlanUsesSwimlane);
    TEST_RUN(_TestAdditionalServicePlanConfigurations);
    TEST_RUN(_TestSingleSourceAndSystemViewPlans);
    TEST_RUN(_TestServicePlanRejectsConflicts);
    return 0;
}

/*************************** End of file ****************************/
