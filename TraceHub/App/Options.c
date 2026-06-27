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
File    : Options.c
Purpose : TraceHub command line option parsing
---------------------------END-OF-HEADER------------------------------
*/

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
  #endif
  #include <windows.h>
#else
  #include <getopt.h>
#endif

#include "Options.h"
#include "CLI.h"

/*********************************************************************
*
*       Defines, fixed
*
**********************************************************************
*/

#if defined(_WIN32)
#define no_argument       0
#define required_argument 1

struct option {
    const char *name;
    int         has_arg;
    int        *flag;
    int         val;
};

static char *optarg = NULL;
static int   optind = 1;

/*********************************************************************
*
*       _WinGetOptFindShort()
*
*  Function description
*    Resolve a short option argument contract for the Windows getopt shim.
*/
static int _WinGetOptFindShort(const char *optstring, int opt) {
    const char *p;

    if (optstring == NULL) {
        return 0;
    }
    p = strchr(optstring, opt);
    if (p == NULL) {
        return 0;
    }
    return (p[1] == ':') ? required_argument : no_argument;
}

/*********************************************************************
*
*       getopt_long()
*
*  Function description
*    Minimal getopt_long-compatible parser used on Windows builds.
*/
static int getopt_long(int argc, char * const argv[],
                       const char *optstring,
                       const struct option *longopts,
                       int *longindex) {
    const char *arg;
    const char *name;
    const char *value;
    size_t      name_len;
    int         i;
    int         need_arg;

    optarg = NULL;
    if (optind >= argc) {
        return -1;
    }

    arg = argv[optind];
    if (arg == NULL || arg[0] != '-' || arg[1] == '\0') {
        return -1;
    }
    if (strcmp(arg, "--") == 0) {
        optind++;
        return -1;
    }

    if (arg[1] == '-') {
        name = arg + 2;
        value = strchr(name, '=');
        name_len = (value == NULL) ? strlen(name) : (size_t)(value - name);
        if (name_len == 0u) {
            optind++;
            return '?';
        }
        for (i = 0; longopts != NULL && longopts[i].name != NULL; i++) {
            if (strlen(longopts[i].name) == name_len &&
                strncmp(longopts[i].name, name, name_len) == 0) {
                if (longindex != NULL) {
                    *longindex = i;
                }
                if (longopts[i].has_arg == required_argument) {
                    if (value != NULL) {
                        optarg = (char *)(value + 1);
                    } else {
                        optind++;
                        if (optind >= argc) {
                            return '?';
                        }
                        optarg = argv[optind];
                    }
                } else if (value != NULL) {
                    optind++;
                    return '?';
                }
                optind++;
                if (longopts[i].flag != NULL) {
                    *longopts[i].flag = longopts[i].val;
                    return 0;
                }
                return longopts[i].val;
            }
        }
        optind++;
        return '?';
    }

    need_arg = _WinGetOptFindShort(optstring, (unsigned char)arg[1]);
    if (need_arg == 0 && strchr(optstring, (unsigned char)arg[1]) == NULL) {
        optind++;
        return '?';
    }
    if (need_arg == required_argument) {
        if (arg[2] != '\0') {
            optarg = (char *)(arg + 2);
        } else {
            optind++;
            if (optind >= argc) {
                return '?';
            }
            optarg = argv[optind];
        }
    }
    optind++;
    return (unsigned char)arg[1];
}
#endif

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
    {"shm",                        required_argument, NULL, 1001},
    {"telnet-port",                required_argument, NULL, 't'},
    {"systemview-port",            required_argument, NULL, 'S'},
    {"systemview",                 no_argument,       NULL, 1002},
    {"linux",                      no_argument,       NULL, 1003},
    {"rtos",                       no_argument,       NULL, 1004},
    {"linux-channel",              required_argument, NULL, 1005},
    {"rtos-channel",               required_argument, NULL, 1006},
    {"systemview-channel",         required_argument, NULL, 1007},
    {"console",                    no_argument,       NULL, 'c'},
    {"swimlane",                   no_argument,       NULL, 1008},
    {"rtt-timeout-ms",             required_argument, NULL, 1009},
    {"memshm-reset",               no_argument,       NULL, 1010},
    {"log-dir",                    required_argument, NULL, 1011},
    {"swimlane-width",             required_argument, NULL, 1012},
    {NULL,                         0,                 NULL,  0 }
};

/*********************************************************************
*
*       Static functions
*
**********************************************************************
*/

/*********************************************************************
*
*       _ParseU64()
*
*  Function description
*    Parse an unsigned 64-bit integer.
*
*  Parameters
*    str    String to parse
*    value  Parsed value
*    base   Numeric base
*
*  Return value
*    0   Success
*   -1   Invalid input or overflow
*/
static int _ParseU64(const char *str, uint64_t *value, int base) {
    char               *end;
    unsigned long long  parsed;

    if ((str == NULL) || (value == NULL) || (str[0] == '\0') ||
        (str[0] == '-') || (str[0] == '+')) {
        return -1;
    }

    errno = 0;
    parsed = strtoull(str, &end, base);
    if ((errno != 0) || (end == str) || (*end != '\0')) {
        return -1;
    }
#if ULLONG_MAX > UINT64_MAX
    if (parsed > UINT64_MAX) {
        return -1;
    }
#endif

    *value = (uint64_t)parsed;
    return 0;
}

/*********************************************************************
*
*       _ParseSize()
*
*  Function description
*    Parse an unsigned size value.
*
*  Parameters
*    str    String to parse
*    value  Parsed value
*    base   Numeric base
*
*  Return value
*    0   Success
*   -1   Invalid input or overflow
*/
static int _ParseSize(const char *str, size_t *value, int base) {
    uint64_t parsed;

    if ((value == NULL) || (_ParseU64(str, &parsed, base) != 0) || (parsed > SIZE_MAX)) {
        return -1;
    }

    *value = (size_t)parsed;
    return 0;
}

/*********************************************************************
*
*       _ParseUnsigned()
*
*  Function description
*    Parse an unsigned integer.
*
*  Parameters
*    str    String to parse
*    value  Parsed value
*    base   Numeric base
*
*  Return value
*    0   Success
*   -1   Invalid input or overflow
*/
static int _ParseUnsigned(const char *str, unsigned *value, int base) {
    uint64_t parsed;

    if ((value == NULL) || (_ParseU64(str, &parsed, base) != 0) || (parsed > UINT_MAX)) {
        return -1;
    }

    *value = (unsigned)parsed;
    return 0;
}

/*********************************************************************
*
*       _ParsePort()
*
*  Function description
*    Parse a TCP port number.
*
*  Parameters
*    str    String to parse
*    value  Parsed port
*
*  Return value
*    0   Success
*   -1   Invalid input or out of TCP port range
*/
static int _ParsePort(const char *str, unsigned *value) {
    unsigned parsed;

    if ((_ParseUnsigned(str, &parsed, 10) != 0) || (parsed == 0u) || (parsed > 65535u)) {
        return -1;
    }

    *value = parsed;
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
*       TraceHubOptions_Parse()
*
*  Function description
*    Parse command-line tokens into raw user intent.
*
*  Parameters
*    argc     Command-line argument count
*    argv     Command-line argument vector
*    options  Output option structure
*
*  Return value
*    0   Success
*   -1   Invalid command line
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
                if (_ParseU64(optarg, &options->rtt_address, 0) != 0) {
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
                if (_ParseSize(optarg, &options->rtt_region_size, 0) != 0) {
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
                if (_ParsePort(optarg, &options->terminal_port) != 0) {
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
                if (_ParsePort(optarg, &options->systemview_port) != 0) {
                    printf("Error: invalid --systemview-port value: %s\n", optarg);
                    return -1;
                }
                options->systemview_port_specified = true;
                break;

            case 1001:
                if ((optarg == NULL) || (optarg[0] == '\0')) {
                    printf("--shm option requires a non-empty argument\n");
                    return -1;
                }
                options->device_path = optarg;
                options->device_path_specified = true;
                break;

            case 1002:
                options->systemview_requested = true;
                break;

            case 1003:
                options->linux_requested = true;
                break;

            case 1004:
                options->rtos_requested = true;
                break;

            case 1005:
                if (optarg == NULL) {
                    printf("--linux-channel option requires an argument\n");
                    return -1;
                }
                if (_ParseUnsigned(optarg, &options->linux_channel, 10) != 0) {
                    printf("Error: invalid --linux-channel value: %s\n", optarg);
                    return -1;
                }
                options->linux_channel_specified = true;
                break;

            case 1006:
                if (optarg == NULL) {
                    printf("--rtos-channel option requires an argument\n");
                    return -1;
                }
                if (_ParseUnsigned(optarg, &options->rtos_channel, 10) != 0) {
                    printf("Error: invalid --rtos-channel value: %s\n", optarg);
                    return -1;
                }
                options->rtos_channel_specified = true;
                break;

            case 1007:
                if (optarg == NULL) {
                    printf("--systemview-channel option requires an argument\n");
                    return -1;
                }
                if (_ParseUnsigned(optarg, &options->systemview_channel, 10) != 0) {
                    printf("Error: invalid --systemview-channel value: %s\n", optarg);
                    return -1;
                }
                options->systemview_channel_specified = true;
                break;

            case 'c':
                options->console_requested = true;
                break;

            case 1008:
                options->swimlane_requested = true;
                break;

            case 1009:
                if (optarg == NULL) {
                    printf("--rtt-timeout-ms option requires an argument\n");
                    return -1;
                }
                if (_ParseUnsigned(optarg, &options->rtt_search_timeout_ms, 10) != 0) {
                    printf("Error: invalid --rtt-timeout-ms value: %s\n", optarg);
                    return -1;
                }
                options->rtt_search_timeout_specified = true;
                break;

            case 1010:
#if defined(RTTMEM_USE_MEMSHM)
                options->memshm_reset_requested = true;
#else
                printf("Error: --memshm-reset requires a MEMSHM backend build.\n");
                return -1;
#endif
                break;

            case 1011:
                if ((optarg == NULL) || (optarg[0] == '\0')) {
                    printf("--log-dir option requires a non-empty argument\n");
                    return -1;
                }
                options->log_dir = optarg;
                options->log_dir_specified = true;
                break;

            case 1012:
                if (optarg == NULL) {
                    printf("--swimlane-width option requires an argument\n");
                    return -1;
                }
                if (_ParseUnsigned(optarg, &options->swimlane_width, 10) != 0) {
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

    return 0;
}

/*************************** End of file ****************************/
