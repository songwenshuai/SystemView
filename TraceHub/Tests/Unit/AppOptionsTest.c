/*********************************************************************
*                             CineLogic                              *
*                      RTT Trace and Debug Bridge                    *
**********************************************************************
----------------------------------------------------------------------
File    : AppOptionsTest.c
Purpose : Unit checks for application option parsing
---------------------------END-OF-HEADER------------------------------
*/

#include <string.h>

#include "Options.h"
#include "TestCommon.h"

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

static int _TestOptionsParseAdditionalInputs(void) {
    TraceHubOptions_t options;
    char *argv[] = {
        "tracehub",
        "--linux",
        "--linux-channel", "3",
        "--console",
        "--swimlane",
        "--systemview-channel", "4",
        "--rtt-timeout-ms", "250",
        "--swimlane-width", "160"
    };
    char *bad_addr_argv[] = { "tracehub", "--addr", "xyz" };
    char *empty_addr_argv[] = { "tracehub", "--addr", "" };
    char *bad_size_argv[] = { "tracehub", "--size", "bad" };
    char *empty_size_argv[] = { "tracehub", "--size", "" };
    char *empty_shm_argv[] = { "tracehub", "--shm", "" };
    char *bad_sysview_port_argv[] = { "tracehub", "--systemview-port", "70000" };
    char *bad_linux_channel_argv[] = { "tracehub", "--linux-channel", "bad" };
    char *bad_rtos_channel_argv[] = { "tracehub", "--rtos-channel", "bad" };
    char *bad_sysview_channel_argv[] = { "tracehub", "--systemview-channel", "bad" };
    char *bad_timeout_argv[] = { "tracehub", "--rtt-timeout-ms", "bad" };
    char *empty_log_dir_argv[] = { "tracehub", "--log-dir", "" };
    char *bad_width_argv[] = { "tracehub", "--swimlane-width", "bad" };
    char *unknown_argv[] = { "tracehub", "--unknown-option" };

    TEST_ASSERT(TraceHubOptions_Parse((int)(sizeof(argv) / sizeof(argv[0])),
                                      argv,
                                      &options) == 0);
    TEST_ASSERT(options.linux_requested);
    TEST_ASSERT(options.linux_channel_specified);
    TEST_ASSERT(options.linux_channel == 3u);
    TEST_ASSERT(options.console_requested);
    TEST_ASSERT(options.swimlane_requested);
    TEST_ASSERT(options.systemview_channel_specified);
    TEST_ASSERT(options.systemview_channel == 4u);
    TEST_ASSERT(options.rtt_search_timeout_specified);
    TEST_ASSERT(options.rtt_search_timeout_ms == 250u);
    TEST_ASSERT(options.swimlane_width_specified);
    TEST_ASSERT(options.swimlane_width == 160u);

    TEST_ASSERT(TraceHubOptions_Parse(3, bad_addr_argv, &options) == -1);
    TEST_ASSERT(TraceHubOptions_Parse(3, empty_addr_argv, &options) == -1);
    TEST_ASSERT(TraceHubOptions_Parse(3, bad_size_argv, &options) == -1);
    TEST_ASSERT(TraceHubOptions_Parse(3, empty_size_argv, &options) == -1);
    TEST_ASSERT(TraceHubOptions_Parse(3, empty_shm_argv, &options) == -1);
    TEST_ASSERT(TraceHubOptions_Parse(3, bad_sysview_port_argv, &options) == -1);
    TEST_ASSERT(TraceHubOptions_Parse(3, bad_linux_channel_argv, &options) == -1);
    TEST_ASSERT(TraceHubOptions_Parse(3, bad_rtos_channel_argv, &options) == -1);
    TEST_ASSERT(TraceHubOptions_Parse(3, bad_sysview_channel_argv, &options) == -1);
    TEST_ASSERT(TraceHubOptions_Parse(3, bad_timeout_argv, &options) == -1);
    TEST_ASSERT(TraceHubOptions_Parse(3, empty_log_dir_argv, &options) == -1);
    TEST_ASSERT(TraceHubOptions_Parse(3, bad_width_argv, &options) == -1);
    TEST_ASSERT(TraceHubOptions_Parse(2, unknown_argv, &options) == -1);
    return 0;
}

int main(void) {
    TEST_RUN(_TestOptionsParseFullConfiguration);
    TEST_RUN(_TestOptionsEarlyExitAndInvalidInputs);
    TEST_RUN(_TestOptionsParseAdditionalInputs);
    return 0;
}

/*************************** End of file ****************************/
