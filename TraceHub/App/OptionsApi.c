/*********************************************************************
*                             CineLogic                              *
*                      RTT Trace and Debug Bridge                    *
**********************************************************************
----------------------------------------------------------------------
File    : OptionsApi.c
Purpose : TraceHub command line option parsing
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

#include "Options.h"
#include "Options_internal.h"
#include "CLI.h"

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

static const char _optstring[] = "hva:s:t:S:c";

static const struct option _options[] = {
    {"help",                       no_argument,       NULL, 'h'},
    {"version",                    no_argument,       NULL, 'v'},
    {"addr",                       required_argument, NULL, 'a'},
    {"size",                       required_argument, NULL, 's'},
    {"shm",                        required_argument, NULL, TRACEHUB_OPT_SHM},
    {"telnet-port",                required_argument, NULL, 't'},
    {"systemview-port",            required_argument, NULL, 'S'},
    {"systemview",                 no_argument,       NULL, TRACEHUB_OPT_SYSTEMVIEW},
    {"linux",                      no_argument,       NULL, TRACEHUB_OPT_LINUX},
    {"rtos",                       no_argument,       NULL, TRACEHUB_OPT_RTOS},
    {"linux-channel",              required_argument, NULL, TRACEHUB_OPT_LINUX_CHANNEL},
    {"rtos-channel",               required_argument, NULL, TRACEHUB_OPT_RTOS_CHANNEL},
    {"systemview-channel",         required_argument, NULL, TRACEHUB_OPT_SYSTEMVIEW_CHANNEL},
    {"console",                    no_argument,       NULL, 'c'},
    {"swimlane",                   no_argument,       NULL, TRACEHUB_OPT_SWIMLANE},
    {"rtt-timeout-ms",             required_argument, NULL, TRACEHUB_OPT_RTT_TIMEOUT_MS},
    {"memshm-reset",               no_argument,       NULL, TRACEHUB_OPT_MEMSHM_RESET},
    {"log-dir",                    required_argument, NULL, TRACEHUB_OPT_LOG_DIR},
    {"swimlane-width",             required_argument, NULL, TRACEHUB_OPT_SWIMLANE_WIDTH},
    {NULL,                         0,                 NULL,  0 }
};

/*********************************************************************
*
*       Public functions
*
**********************************************************************
*/

/*********************************************************************
*
*       TraceHubOptions_Parse()
*
*  Function description
*    Parse command-line tokens into raw user intent.
*
*  Parameters
*    argc     Command-line argument count.
*    argv     Command-line argument vector.
*    options  Output option structure.
*
*  Return value
*    0   Success.
*   -1   Invalid command line.
*/
int TraceHubOptions_Parse(int argc, char *argv[], TraceHubOptions_t *options) {
    int opt;
    int lindex;

    if (options == NULL) {
        return -1;
    }
    memset(options, 0, sizeof(*options));

    optind = 1;
    lindex = -1;
#if !defined(_WIN32)
    opterr = 0;
#endif

    while ((opt = getopt_long(argc, argv, _optstring, _options, &lindex)) != -1) {
        switch (opt) {
            case 'h':
                options->help_requested = true;
                return 0;

            case 'v':
                options->version_requested = true;
                return 0;

            case 'a':
                if ((optarg == NULL) || (optarg[0] == '\0')) {
                    printf("--addr option requires a non-empty argument\n");
                    return -1;
                }
                if (TraceHubOptions_ParseU64(optarg, &options->rtt_address, 0) != 0) {
                    printf("Error: invalid --addr value: %s\n", optarg);
                    return -1;
                }
                options->rtt_address_specified = true;
                break;

            case 's':
                if ((optarg == NULL) || (optarg[0] == '\0')) {
                    printf("--size option requires a non-empty argument\n");
                    return -1;
                }
                if (TraceHubOptions_ParseSize(optarg, &options->rtt_region_size, 0) != 0) {
                    printf("Error: invalid --size value: %s\n", optarg);
                    return -1;
                }
                options->rtt_region_size_specified = true;
                break;

            case 't':
                if (optarg == NULL) {
                    printf("--telnet-port option requires an argument\n");
                    return -1;
                }
                if (TraceHubOptions_ParsePort(optarg, &options->terminal_port) != 0) {
                    printf("Error: invalid --telnet-port value: %s\n", optarg);
                    return -1;
                }
                options->terminal_port_specified = true;
                break;

            case 'S':
                if (optarg == NULL) {
                    printf("--systemview-port option requires an argument\n");
                    return -1;
                }
                if (TraceHubOptions_ParsePort(optarg, &options->systemview_port) != 0) {
                    printf("Error: invalid --systemview-port value: %s\n", optarg);
                    return -1;
                }
                options->systemview_port_specified = true;
                break;

            case TRACEHUB_OPT_SHM:
                if ((optarg == NULL) || (optarg[0] == '\0')) {
                    printf("--shm option requires a non-empty argument\n");
                    return -1;
                }
                options->device_path = optarg;
                options->device_path_specified = true;
                break;

            case TRACEHUB_OPT_SYSTEMVIEW:
                options->systemview_requested = true;
                break;

            case TRACEHUB_OPT_LINUX:
                options->linux_requested = true;
                break;

            case TRACEHUB_OPT_RTOS:
                options->rtos_requested = true;
                break;

            case TRACEHUB_OPT_LINUX_CHANNEL:
                if (optarg == NULL) {
                    printf("--linux-channel option requires an argument\n");
                    return -1;
                }
                if (TraceHubOptions_ParseUnsigned(optarg, &options->linux_channel, 10) != 0) {
                    printf("Error: invalid --linux-channel value: %s\n", optarg);
                    return -1;
                }
                options->linux_channel_specified = true;
                break;

            case TRACEHUB_OPT_RTOS_CHANNEL:
                if (optarg == NULL) {
                    printf("--rtos-channel option requires an argument\n");
                    return -1;
                }
                if (TraceHubOptions_ParseUnsigned(optarg, &options->rtos_channel, 10) != 0) {
                    printf("Error: invalid --rtos-channel value: %s\n", optarg);
                    return -1;
                }
                options->rtos_channel_specified = true;
                break;

            case TRACEHUB_OPT_SYSTEMVIEW_CHANNEL:
                if (optarg == NULL) {
                    printf("--systemview-channel option requires an argument\n");
                    return -1;
                }
                if (TraceHubOptions_ParseUnsigned(optarg, &options->systemview_channel, 10) != 0) {
                    printf("Error: invalid --systemview-channel value: %s\n", optarg);
                    return -1;
                }
                options->systemview_channel_specified = true;
                break;

            case 'c':
                options->console_requested = true;
                break;

            case TRACEHUB_OPT_SWIMLANE:
                options->swimlane_requested = true;
                break;

            case TRACEHUB_OPT_RTT_TIMEOUT_MS:
                if (optarg == NULL) {
                    printf("--rtt-timeout-ms option requires an argument\n");
                    return -1;
                }
                if (TraceHubOptions_ParseUnsigned(optarg, &options->rtt_search_timeout_ms, 10) != 0) {
                    printf("Error: invalid --rtt-timeout-ms value: %s\n", optarg);
                    return -1;
                }
                options->rtt_search_timeout_specified = true;
                break;

            case TRACEHUB_OPT_MEMSHM_RESET:
#if defined(RTTMEM_USE_MEMSHM)
                options->memshm_reset_requested = true;
#else
                printf("Error: --memshm-reset requires a MEMSHM backend build.\n");
                return -1;
#endif
                break;

            case TRACEHUB_OPT_LOG_DIR:
                if ((optarg == NULL) || (optarg[0] == '\0')) {
                    printf("--log-dir option requires a non-empty argument\n");
                    return -1;
                }
                options->log_dir = optarg;
                options->log_dir_specified = true;
                break;

            case TRACEHUB_OPT_SWIMLANE_WIDTH:
                if (optarg == NULL) {
                    printf("--swimlane-width option requires an argument\n");
                    return -1;
                }
                if (TraceHubOptions_ParseUnsigned(optarg, &options->swimlane_width, 10) != 0) {
                    printf("Error: invalid --swimlane-width value: %s\n", optarg);
                    return -1;
                }
                options->swimlane_width_specified = true;
                break;

            default:
                printf("Invalid option.\n");
                CLI_PrintUsage();
                return -1;
        }
    }

    if (optind < argc) {
        printf("Error: unexpected positional argument: %s\n", argv[optind]);
        return -1;
    }

    return 0;
}

/*************************** End of file ****************************/
