/*********************************************************************
*                             CineLogic                              *
*                      RTT Trace and Debug Bridge                    *
**********************************************************************
----------------------------------------------------------------------
File    : AppRunLoopTest.c
Purpose : Unit checks for application run loop APIs
---------------------------END-OF-HEADER------------------------------
*/

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "CLI.h"
#include "TraceHubSignal.h"
#include "RunLoop.h"
#include "LogCollector.h"
#include "LogMerger.h"
#include "SwimLaneRenderer.h"
#include "TestCommon.h"

static int      _stub_core_log_start_result;
static bool     _stub_core_log_fatal;
static int      _stub_terminal_start_result;
static bool     _stub_terminal_fatal;
static unsigned _stub_terminal_start_count;
static int      _stub_systemview_start_result;
static bool     _stub_systemview_fatal;
static unsigned _stub_systemview_start_count;
static int      _stub_logcollector_init_result;
static int      _stub_logcollector_start_result;
static bool     _stub_logcollector_running;
static bool     _stub_logcollector_fatal;
static bool     _stub_logcollector_invoke_callback;
static unsigned _stub_logcollector_start_count;
static unsigned _stub_logcollector_stop_count;
static unsigned _stub_logcollector_cleanup_count;
static int      _stub_logmerger_init_result;
static int      _stub_logmerger_process_result;
static bool     _stub_logmerger_call_output;
static int      _stub_logmerger_write_result;
static int      _stub_logmerger_flush_ready_result;
static int      _stub_logmerger_flush_result;
static bool     _stub_logmerger_file_error;
static unsigned _stub_logmerger_process_count;
static unsigned _stub_logmerger_write_count;
static unsigned _stub_logmerger_cleanup_count;
static int      _stub_swimlane_init_result;
static int      _stub_swimlane_render_result;
static unsigned _stub_swimlane_render_count;
static unsigned _stub_swimlane_cleanup_count;
static bool     _stub_stop_on_sleep;
static unsigned _stub_sleep_count;
static LogEntry_t _stub_log_entry;

static void _ResetRuntimeStubs(void) {
    _stub_core_log_start_result = 0;
    _stub_core_log_fatal = false;
    _stub_terminal_start_result = 0;
    _stub_terminal_fatal = false;
    _stub_terminal_start_count = 0u;
    _stub_systemview_start_result = 0;
    _stub_systemview_fatal = false;
    _stub_systemview_start_count = 0u;
    _stub_logcollector_init_result = 0;
    _stub_logcollector_start_result = 0;
    _stub_logcollector_running = true;
    _stub_logcollector_fatal = false;
    _stub_logcollector_invoke_callback = false;
    _stub_logcollector_start_count = 0u;
    _stub_logcollector_stop_count = 0u;
    _stub_logcollector_cleanup_count = 0u;
    _stub_logmerger_init_result = 0;
    _stub_logmerger_process_result = 0;
    _stub_logmerger_call_output = false;
    _stub_logmerger_write_result = 0;
    _stub_logmerger_flush_ready_result = 0;
    _stub_logmerger_flush_result = 0;
    _stub_logmerger_file_error = false;
    _stub_logmerger_process_count = 0u;
    _stub_logmerger_write_count = 0u;
    _stub_logmerger_cleanup_count = 0u;
    _stub_swimlane_init_result = 0;
    _stub_swimlane_render_result = 0;
    _stub_swimlane_render_count = 0u;
    _stub_swimlane_cleanup_count = 0u;
    _stub_stop_on_sleep = false;
    _stub_sleep_count = 0u;
    memset(&_stub_log_entry, 0, sizeof(_stub_log_entry));
    _stub_log_entry.valid = true;
    _stub_log_entry.source = LOG_SOURCE_LINUX;
}

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
    _stub_sleep_count++;
    if (_stub_stop_on_sleep) {
        volatile sig_atomic_t *run_flag;

        run_flag = TraceHubSignal_GetRunFlag();
        if (run_flag != NULL) {
            *run_flag = 0;
        }
    }
}

int CoreLogRecorder_Start(void) {
    return _stub_core_log_start_result;
}

bool CoreLogRecorder_HasFatalError(void) {
    return _stub_core_log_fatal;
}

int Terminal_Start(void) {
    _stub_terminal_start_count++;
    return _stub_terminal_start_result;
}

bool Terminal_HasFatalError(void) {
    return _stub_terminal_fatal;
}

int SystemView_Start(void) {
    _stub_systemview_start_count++;
    return _stub_systemview_start_result;
}

bool SystemView_HasFatalError(void) {
    return _stub_systemview_fatal;
}

int LogCollector_Init(LogCollector_Config_t *config) {
    (void)config;
    return _stub_logcollector_init_result;
}

void LogCollector_Cleanup(void) {
    _stub_logcollector_cleanup_count++;
}

int LogCollector_Start(LogCollector_Callback_t callback, void *user_data) {
    _stub_logcollector_start_count++;
    if (_stub_logcollector_start_result != 0) {
        return _stub_logcollector_start_result;
    }
    if (_stub_logcollector_invoke_callback && callback != NULL) {
        return callback(&_stub_log_entry, user_data);
    }
    return 0;
}

void LogCollector_Stop(void) {
    _stub_logcollector_stop_count++;
}

bool LogCollector_IsRunning(void) {
    return _stub_logcollector_running;
}

bool LogCollector_HasFatalError(void) {
    return _stub_logcollector_fatal;
}

int LogMerger_Init(LogMerger_Config_t *config) {
    (void)config;
    return _stub_logmerger_init_result;
}

void LogMerger_Cleanup(void) {
    _stub_logmerger_cleanup_count++;
}

int LogMerger_Process(LogEntry_t *entry, LogMerger_Output_t output, void *user_data) {
    _stub_logmerger_process_count++;
    if (_stub_logmerger_process_result != 0) {
        LogEntry_Destroy(entry);
        return _stub_logmerger_process_result;
    }
    if (_stub_logmerger_call_output && output != NULL) {
        return output(entry, user_data);
    }
    LogEntry_Destroy(entry);
    return 0;
}

int LogMerger_WriteEntry(const LogEntry_t *entry) {
    (void)entry;
    _stub_logmerger_write_count++;
    return _stub_logmerger_write_result;
}

int LogMerger_FlushReady(LogMerger_Output_t output, void *user_data) {
    (void)output;
    (void)user_data;
    return _stub_logmerger_flush_ready_result;
}

int LogMerger_Flush(LogMerger_Output_t output, void *user_data) {
    (void)output;
    (void)user_data;
    return _stub_logmerger_flush_result;
}

bool LogMerger_HasFileError(void) {
    return _stub_logmerger_file_error;
}

int SwimLane_Init(SwimLane_Config_t *config) {
    (void)config;
    return _stub_swimlane_init_result;
}

void SwimLane_Cleanup(void) {
    _stub_swimlane_cleanup_count++;
}

int SwimLane_RenderEntry(const LogEntry_t *entry) {
    (void)entry;
    _stub_swimlane_render_count++;
    return _stub_swimlane_render_result;
}

void LogEntry_Destroy(LogEntry_t *entry) {
    (void)entry;
}

static int _TestCliSignalAndRunLoopContracts(void) {
    FILE                  *file;
    char                   buffer[512];
    volatile sig_atomic_t *run_flag;
    int                    result;

    _ResetRuntimeStubs();
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

static int _TestSignalNonFatalHandlers(void) {
    volatile sig_atomic_t *run_flag;

    TraceHubSignal_Init();
    run_flag = TraceHubSignal_GetRunFlag();
    TEST_ASSERT(run_flag != NULL);
    TEST_ASSERT(*run_flag == 1);
    TEST_ASSERT(raise(SIGINT) == 0);
    TEST_ASSERT(*run_flag == 0);

    TraceHubSignal_Init();
    TEST_ASSERT(*run_flag == 1);
    TEST_ASSERT(raise(SIGTERM) == 0);
    TEST_ASSERT(*run_flag == 0);

    TraceHubSignal_Init();
    TEST_ASSERT(*run_flag == 1);
    return 0;
}

static void _FillRunLoopConfig(TraceHubRunLoop_Config_t *config) {
    memset(config, 0, sizeof(*config));
    config->linux_channel = 0u;
    config->rtos_channel = 1u;
    config->terminal_channel = 0u;
    config->sysview_channel = 2u;
    config->terminal_port = 2323u;
    config->sysview_port = 19111u;
    config->swimlane_width = 120u;
    config->swimlane_log_prefix = "tracehub_unit_swimlane";
}

static int _TestRunLoopNormalModeSuccess(void) {
    TraceHubRunLoop_Config_t config;

    _ResetRuntimeStubs();
    TraceHubSignal_Init();
    _FillRunLoopConfig(&config);
    config.terminal_enabled = true;
    config.sysview_enabled = true;
    config.core_log_enabled = true;
    config.terminal_network_enabled = true;
    config.systemview_network_enabled = true;
    _stub_stop_on_sleep = true;

    TEST_ASSERT(TraceHubRunLoop_Run(&config) == 0);
    TEST_ASSERT(_stub_terminal_start_count == 1u);
    TEST_ASSERT(_stub_systemview_start_count == 1u);
    TEST_ASSERT(_stub_sleep_count == 1u);
    return 0;
}

static int _TestRunLoopNormalModeFailures(void) {
    TraceHubRunLoop_Config_t config;

    _ResetRuntimeStubs();
    TraceHubSignal_Init();
    _FillRunLoopConfig(&config);
    config.terminal_enabled = true;
    _stub_terminal_start_result = -1;
    TEST_ASSERT(TraceHubRunLoop_Run(&config) == -1);

    _ResetRuntimeStubs();
    TraceHubSignal_Init();
    _FillRunLoopConfig(&config);
    config.core_log_enabled = true;
    _stub_core_log_start_result = -1;
    TEST_ASSERT(TraceHubRunLoop_Run(&config) == -1);

    _ResetRuntimeStubs();
    TraceHubSignal_Init();
    _FillRunLoopConfig(&config);
    config.sysview_enabled = true;
    _stub_systemview_fatal = true;
    TEST_ASSERT(TraceHubRunLoop_Run(&config) == -1);
    return 0;
}

static int _TestRunLoopSwimLaneModeSuccess(void) {
    TraceHubRunLoop_Config_t config;

    _ResetRuntimeStubs();
    TraceHubSignal_Init();
    _FillRunLoopConfig(&config);
    config.swimlane_mode = true;
    config.core_log_enabled = true;
    config.sysview_enabled = true;
    config.systemview_network_enabled = false;
    _stub_logcollector_invoke_callback = true;
    _stub_logmerger_call_output = true;
    _stub_stop_on_sleep = true;

    TEST_ASSERT(TraceHubRunLoop_Run(&config) == 0);
    TEST_ASSERT(_stub_logcollector_start_count == 1u);
    TEST_ASSERT(_stub_logcollector_stop_count == 1u);
    TEST_ASSERT(_stub_logcollector_cleanup_count == 1u);
    TEST_ASSERT(_stub_logmerger_process_count == 1u);
    TEST_ASSERT(_stub_logmerger_write_count == 1u);
    TEST_ASSERT(_stub_logmerger_cleanup_count == 1u);
    TEST_ASSERT(_stub_swimlane_render_count == 1u);
    TEST_ASSERT(_stub_swimlane_cleanup_count == 1u);
    TEST_ASSERT(_stub_systemview_start_count == 1u);
    return 0;
}

static int _TestRunLoopSwimLaneModeFailures(void) {
    TraceHubRunLoop_Config_t config;

    _ResetRuntimeStubs();
    TraceHubSignal_Init();
    _FillRunLoopConfig(&config);
    config.swimlane_mode = true;
    _stub_logmerger_init_result = -1;
    TEST_ASSERT(TraceHubRunLoop_Run(&config) == -1);
    TEST_ASSERT(_stub_logcollector_cleanup_count == 1u);
    TEST_ASSERT(_stub_logmerger_cleanup_count == 0u);

    _ResetRuntimeStubs();
    TraceHubSignal_Init();
    _FillRunLoopConfig(&config);
    config.swimlane_mode = true;
    _stub_logcollector_running = false;
    TEST_ASSERT(TraceHubRunLoop_Run(&config) == -1);
    TEST_ASSERT(_stub_logcollector_stop_count == 1u);
    TEST_ASSERT(_stub_swimlane_cleanup_count == 1u);
    TEST_ASSERT(_stub_logmerger_cleanup_count == 1u);
    TEST_ASSERT(_stub_logcollector_cleanup_count == 1u);
    return 0;
}

static int _TestRunLoopNormalModeAdditionalPaths(void) {
    TraceHubRunLoop_Config_t config;

    _ResetRuntimeStubs();
    TraceHubSignal_Init();
    _FillRunLoopConfig(&config);
    config.only_systemview = true;
    config.sysview_enabled = true;
    config.systemview_network_enabled = true;
    _stub_stop_on_sleep = true;
    TEST_ASSERT(TraceHubRunLoop_Run(&config) == 0);

    _ResetRuntimeStubs();
    TraceHubSignal_Init();
    _FillRunLoopConfig(&config);
    config.console_mode = true;
    config.terminal_enabled = true;
    config.sysview_enabled = true;
    config.systemview_network_enabled = false;
    _stub_stop_on_sleep = true;
    TEST_ASSERT(TraceHubRunLoop_Run(&config) == 0);

    _ResetRuntimeStubs();
    TraceHubSignal_Init();
    _FillRunLoopConfig(&config);
    config.terminal_enabled = true;
    config.terminal_network_enabled = false;
    _stub_stop_on_sleep = true;
    TEST_ASSERT(TraceHubRunLoop_Run(&config) == 0);

    _ResetRuntimeStubs();
    TraceHubSignal_Init();
    _FillRunLoopConfig(&config);
    config.sysview_enabled = true;
    _stub_systemview_start_result = -1;
    TEST_ASSERT(TraceHubRunLoop_Run(&config) == -1);

    _ResetRuntimeStubs();
    TraceHubSignal_Init();
    _FillRunLoopConfig(&config);
    config.terminal_enabled = true;
    _stub_terminal_fatal = true;
    TEST_ASSERT(TraceHubRunLoop_Run(&config) == -1);

    _ResetRuntimeStubs();
    TraceHubSignal_Init();
    _FillRunLoopConfig(&config);
    config.core_log_enabled = true;
    _stub_core_log_fatal = true;
    TEST_ASSERT(TraceHubRunLoop_Run(&config) == -1);
    return 0;
}

static int _TestRunLoopSwimLaneInitializationFailures(void) {
    TraceHubRunLoop_Config_t config;

    _ResetRuntimeStubs();
    TraceHubSignal_Init();
    _FillRunLoopConfig(&config);
    config.swimlane_mode = true;
    _stub_logcollector_init_result = -1;
    TEST_ASSERT(TraceHubRunLoop_Run(&config) == -1);
    TEST_ASSERT(_stub_logcollector_cleanup_count == 0u);

    _ResetRuntimeStubs();
    TraceHubSignal_Init();
    _FillRunLoopConfig(&config);
    config.swimlane_mode = true;
    _stub_swimlane_init_result = -1;
    TEST_ASSERT(TraceHubRunLoop_Run(&config) == -1);
    TEST_ASSERT(_stub_logmerger_cleanup_count == 1u);
    TEST_ASSERT(_stub_logcollector_cleanup_count == 1u);

    _ResetRuntimeStubs();
    TraceHubSignal_Init();
    _FillRunLoopConfig(&config);
    config.swimlane_mode = true;
    _stub_logcollector_start_result = -1;
    TEST_ASSERT(TraceHubRunLoop_Run(&config) == -1);
    TEST_ASSERT(_stub_swimlane_cleanup_count == 1u);
    TEST_ASSERT(_stub_logmerger_cleanup_count == 1u);
    TEST_ASSERT(_stub_logcollector_cleanup_count == 1u);

    _ResetRuntimeStubs();
    TraceHubSignal_Init();
    _FillRunLoopConfig(&config);
    config.swimlane_mode = true;
    config.core_log_enabled = true;
    _stub_core_log_start_result = -1;
    TEST_ASSERT(TraceHubRunLoop_Run(&config) == -1);
    TEST_ASSERT(_stub_logcollector_stop_count == 1u);
    TEST_ASSERT(_stub_swimlane_cleanup_count == 1u);
    TEST_ASSERT(_stub_logmerger_cleanup_count == 1u);
    TEST_ASSERT(_stub_logcollector_cleanup_count == 1u);

    _ResetRuntimeStubs();
    TraceHubSignal_Init();
    _FillRunLoopConfig(&config);
    config.swimlane_mode = true;
    config.sysview_enabled = true;
    _stub_systemview_start_result = -1;
    TEST_ASSERT(TraceHubRunLoop_Run(&config) == -1);
    TEST_ASSERT(_stub_logcollector_stop_count == 1u);
    TEST_ASSERT(_stub_swimlane_cleanup_count == 1u);
    TEST_ASSERT(_stub_logmerger_cleanup_count == 1u);
    TEST_ASSERT(_stub_logcollector_cleanup_count == 1u);
    return 0;
}

static int _TestRunLoopSwimLaneRuntimeFailures(void) {
    TraceHubRunLoop_Config_t config;

    _ResetRuntimeStubs();
    TraceHubSignal_Init();
    _FillRunLoopConfig(&config);
    config.swimlane_mode = true;
    config.core_log_enabled = true;
    _stub_core_log_fatal = true;
    TEST_ASSERT(TraceHubRunLoop_Run(&config) == -1);
    TEST_ASSERT(_stub_logcollector_stop_count == 1u);

    _ResetRuntimeStubs();
    TraceHubSignal_Init();
    _FillRunLoopConfig(&config);
    config.swimlane_mode = true;
    _stub_logcollector_fatal = true;
    TEST_ASSERT(TraceHubRunLoop_Run(&config) == -1);
    TEST_ASSERT(_stub_logcollector_stop_count == 1u);

    _ResetRuntimeStubs();
    TraceHubSignal_Init();
    _FillRunLoopConfig(&config);
    config.swimlane_mode = true;
    config.sysview_enabled = true;
    _stub_systemview_fatal = true;
    TEST_ASSERT(TraceHubRunLoop_Run(&config) == -1);
    TEST_ASSERT(_stub_logcollector_stop_count == 1u);

    _ResetRuntimeStubs();
    TraceHubSignal_Init();
    _FillRunLoopConfig(&config);
    config.swimlane_mode = true;
    _stub_logmerger_flush_ready_result = -1;
    _stub_stop_on_sleep = true;
    TEST_ASSERT(TraceHubRunLoop_Run(&config) == -1);
    TEST_ASSERT(_stub_logcollector_stop_count == 1u);

    _ResetRuntimeStubs();
    TraceHubSignal_Init();
    _FillRunLoopConfig(&config);
    config.swimlane_mode = true;
    _stub_logcollector_invoke_callback = true;
    _stub_logmerger_process_result = -1;
    TEST_ASSERT(TraceHubRunLoop_Run(&config) == -1);
    TEST_ASSERT(_stub_logmerger_process_count == 1u);

    _ResetRuntimeStubs();
    TraceHubSignal_Init();
    _FillRunLoopConfig(&config);
    config.swimlane_mode = true;
    _stub_logcollector_invoke_callback = true;
    _stub_logmerger_call_output = true;
    _stub_logmerger_write_result = -1;
    TEST_ASSERT(TraceHubRunLoop_Run(&config) == -1);
    TEST_ASSERT(_stub_logmerger_write_count == 1u);
    return 0;
}

static int _TestRunLoopSwimLaneShutdownFailures(void) {
    TraceHubRunLoop_Config_t config;

    _ResetRuntimeStubs();
    TraceHubSignal_Init();
    _FillRunLoopConfig(&config);
    config.swimlane_mode = true;
    _stub_stop_on_sleep = true;
    _stub_logmerger_flush_result = -1;
    TEST_ASSERT(TraceHubRunLoop_Run(&config) == -1);
    TEST_ASSERT(_stub_logcollector_stop_count == 1u);
    TEST_ASSERT(_stub_swimlane_cleanup_count == 1u);
    TEST_ASSERT(_stub_logmerger_cleanup_count == 1u);
    TEST_ASSERT(_stub_logcollector_cleanup_count == 1u);

    _ResetRuntimeStubs();
    TraceHubSignal_Init();
    _FillRunLoopConfig(&config);
    config.swimlane_mode = true;
    _stub_stop_on_sleep = true;
    _stub_logmerger_file_error = true;
    TEST_ASSERT(TraceHubRunLoop_Run(&config) == -1);
    TEST_ASSERT(_stub_logcollector_stop_count == 1u);
    TEST_ASSERT(_stub_swimlane_cleanup_count == 1u);
    TEST_ASSERT(_stub_logmerger_cleanup_count == 1u);
    TEST_ASSERT(_stub_logcollector_cleanup_count == 1u);
    return 0;
}

int main(void) {
    TEST_RUN(_TestCliSignalAndRunLoopContracts);
    TEST_RUN(_TestSignalNonFatalHandlers);
    TEST_RUN(_TestRunLoopNormalModeSuccess);
    TEST_RUN(_TestRunLoopNormalModeFailures);
    TEST_RUN(_TestRunLoopSwimLaneModeSuccess);
    TEST_RUN(_TestRunLoopSwimLaneModeFailures);
    TEST_RUN(_TestRunLoopNormalModeAdditionalPaths);
    TEST_RUN(_TestRunLoopSwimLaneInitializationFailures);
    TEST_RUN(_TestRunLoopSwimLaneRuntimeFailures);
    TEST_RUN(_TestRunLoopSwimLaneShutdownFailures);
    return 0;
}

/*************************** End of file ****************************/
