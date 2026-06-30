/*********************************************************************
*                             CineLogic                              *
*                      RTT Trace and Debug Bridge                    *
**********************************************************************
----------------------------------------------------------------------
File    : AppApiTest.c
Purpose : Unit checks for application public APIs
---------------------------END-OF-HEADER------------------------------
*/

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "CLI.h"
#include "Options.h"
#include "ServicePlan.h"
#include "TraceHubSignal.h"
#include "RunLoop.h"
#include "LogCollector.h"
#include "LogMerger.h"
#include "SwimLaneRenderer.h"
#include "TestCommon.h"

void LOG_Debug(const char *file, int line, const char *function, const char *sFormat, ...) {
    (void)file;
    (void)line;
    (void)function;
    (void)sFormat;
}

void LOG_Error(const char *sFormat, ...) {
    (void)sFormat;
}

void LOG_Warn(const char *sFormat, ...) {
    (void)sFormat;
}

void SYS_Sleep(unsigned ms) {
    (void)ms;
}

int CoreLogRecorder_Start(void) {
    return 0;
}

bool CoreLogRecorder_HasFatalError(void) {
    return false;
}

int Terminal_Start(void) {
    return 0;
}

bool Terminal_HasFatalError(void) {
    return false;
}

int SystemView_Start(void) {
    return 0;
}

bool SystemView_HasFatalError(void) {
    return false;
}

int LogCollector_Init(LogCollector_Config_t *config) {
    (void)config;
    return 0;
}

void LogCollector_Cleanup(void) {
}

int LogCollector_Start(LogCollector_Callback_t callback, void *user_data) {
    (void)callback;
    (void)user_data;
    return 0;
}

void LogCollector_Stop(void) {
}

bool LogCollector_IsRunning(void) {
    return true;
}

bool LogCollector_HasFatalError(void) {
    return false;
}

int LogMerger_Init(LogMerger_Config_t *config) {
    (void)config;
    return 0;
}

void LogMerger_Cleanup(void) {
}

int LogMerger_Process(LogEntry_t *entry, LogMerger_Output_t output, void *user_data) {
    (void)output;
    (void)user_data;
    LogEntry_Destroy(entry);
    return 0;
}

int LogMerger_WriteEntry(const LogEntry_t *entry) {
    (void)entry;
    return 0;
}

int LogMerger_FlushReady(LogMerger_Output_t output, void *user_data) {
    (void)output;
    (void)user_data;
    return 0;
}

int LogMerger_Flush(LogMerger_Output_t output, void *user_data) {
    (void)output;
    (void)user_data;
    return 0;
}

bool LogMerger_HasFileError(void) {
    return false;
}

int SwimLane_Init(SwimLane_Config_t *config) {
    (void)config;
    return 0;
}

void SwimLane_Cleanup(void) {
}

int SwimLane_RenderEntry(const LogEntry_t *entry) {
    (void)entry;
    return 0;
}

void LogEntry_Destroy(LogEntry_t *entry) {
    (void)entry;
}

static int _TestOptionsParseFullConfiguration(void) {
    TraceHubOptions_t options;
    char *argv[] = {
        "tracehub",
        "--addr", "0x1000",
        "--size", "4096",
        "--shm", "/rtt_sim",
        "--telnet-port", "2323",
        "--systemview-port", "19111",
        "--systemview",
        "--rtos",
        "--rtos-channel", "5",
        "--systemview-channel", "6",
        "--rtt-timeout-ms", "150",
        "--memshm-reset",
        "--log-dir", ".",
        "--swimlane-width", "140"
    };

    TEST_ASSERT(TraceHubOptions_Parse((int)(sizeof(argv) / sizeof(argv[0])),
                                      argv,
                                      &options) == 0);
    TEST_ASSERT(options.rtt_address_specified);
    TEST_ASSERT(options.rtt_address == 0x1000u);
    TEST_ASSERT(options.rtt_region_size_specified);
    TEST_ASSERT(options.rtt_region_size == 4096u);
    TEST_ASSERT(options.device_path_specified);
    TEST_ASSERT(strcmp(options.device_path, "/rtt_sim") == 0);
    TEST_ASSERT(options.terminal_port_specified);
    TEST_ASSERT(options.terminal_port == 2323u);
    TEST_ASSERT(options.systemview_port_specified);
    TEST_ASSERT(options.systemview_port == 19111u);
    TEST_ASSERT(options.systemview_requested);
    TEST_ASSERT(options.rtos_requested);
    TEST_ASSERT(options.rtos_channel_specified);
    TEST_ASSERT(options.rtos_channel == 5u);
    TEST_ASSERT(options.systemview_channel_specified);
    TEST_ASSERT(options.systemview_channel == 6u);
    TEST_ASSERT(options.rtt_search_timeout_specified);
    TEST_ASSERT(options.rtt_search_timeout_ms == 150u);
    TEST_ASSERT(options.memshm_reset_requested);
    TEST_ASSERT(options.log_dir_specified);
    TEST_ASSERT(strcmp(options.log_dir, ".") == 0);
    TEST_ASSERT(options.swimlane_width_specified);
    TEST_ASSERT(options.swimlane_width == 140u);
    return 0;
}

static int _TestOptionsEarlyExitAndInvalidInputs(void) {
    TraceHubOptions_t options;
    char *help_argv[] = { "tracehub", "--help", "--bad" };
    char *version_argv[] = { "tracehub", "--version" };
    char *bad_port_argv[] = { "tracehub", "--telnet-port", "0" };
    char *bad_positional_argv[] = { "tracehub", "unexpected" };

    TEST_ASSERT(TraceHubOptions_Parse(3, help_argv, &options) == 0);
    TEST_ASSERT(options.help_requested);
    TEST_ASSERT(TraceHubOptions_Parse(2, version_argv, &options) == 0);
    TEST_ASSERT(options.version_requested);
    TEST_ASSERT(TraceHubOptions_Parse(3, bad_port_argv, &options) == -1);
    TEST_ASSERT(TraceHubOptions_Parse(2, bad_positional_argv, &options) == -1);
    TEST_ASSERT(TraceHubOptions_Parse(0, NULL, NULL) == -1);
    return 0;
}

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
    options.linux_requested = true;
    options.rtos_requested = true;
    TEST_ASSERT(TraceHubServicePlan_Build(&options, &run_flag, &plan) == -1);

    memset(&options, 0, sizeof(options));
    options.swimlane_requested = true;
    options.systemview_requested = true;
    options.systemview_channel_specified = true;
    options.systemview_channel = RTT_BRIDGE_DEFAULT_LINUX_CHANNEL;
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

static int _TestCliSignalAndRunLoopContracts(void) {
    FILE                  *file;
    char                   buffer[512];
    volatile sig_atomic_t *run_flag;
    int                    result;

    file = tmpfile();
    TEST_ASSERT(file != NULL);
    CLI_PrintBanner(file);
    result = Test_ReadTmpFile(file, buffer, sizeof(buffer));
    TEST_ASSERT(fclose(file) == 0);
    TEST_ASSERT(result == 0);
    TEST_ASSERT(strstr(buffer, "TraceHub") != NULL);

    CLI_PrintBanner(NULL);
    CLI_PrintUsage();

    TraceHubSignal_Init();
    run_flag = TraceHubSignal_GetRunFlag();
    TEST_ASSERT(run_flag != NULL);
    TEST_ASSERT(*run_flag == 1);

    TEST_ASSERT(TraceHubRunLoop_Run(NULL) == -1);
    return 0;
}

int main(void) {
    TEST_RUN(_TestOptionsParseFullConfiguration);
    TEST_RUN(_TestOptionsEarlyExitAndInvalidInputs);
    TEST_RUN(_TestDefaultServicePlanUsesSwimlane);
    TEST_RUN(_TestSingleSourceAndSystemViewPlans);
    TEST_RUN(_TestServicePlanRejectsConflicts);
    TEST_RUN(_TestCliSignalAndRunLoopContracts);
    return 0;
}

/*************************** End of file ****************************/
