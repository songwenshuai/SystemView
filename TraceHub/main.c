/*********************************************************************
*                                                                    *
*                Copyright (C) 2023 xrTest Inc.                      *
*                      All rights reserved                           *
*                                                                    *
**********************************************************************
----------------------------------------------------------------------
File    : main.c
Author  : songwenshuai <songwenshuai@gmail.com>
Purpose : Multi-channel RTT bridge supporting Terminal and SystemView.

          This application provides TCP server interfaces for SEGGER
          Real-Time Transfer (RTT) debugging protocol:
          - Terminal service (default port 19021, channel 0)
          - SystemView service (default port 19111, channel 2)
          Implementation of SEGGER real-time transfer (RTT) which
          allows real-time communication on targets which support
          debugger memory accesses while the CPU is running.
Revision: $Rev: <0.0.1> (org: 25842) $

Additional information:
          Type "int" is assumed to be 32-bits in size
          H->T    Host to target communication
          T->H    Target to host communication

          RTT channel 0 is always present and reserved for Terminal usage.
          Name is fixed to "Terminal"

          Effective buffer size: SizeOfBuffer - 1

          WrOff == RdOff:       Buffer is empty
          WrOff == (RdOff - 1): Buffer is full
          WrOff >  RdOff:       Free space includes wrap-around
          WrOff <  RdOff:       Used space includes wrap-around
          (WrOff == (SizeOfBuffer - 1)) && (RdOff == 0):
                                Buffer full and wrap-around after next byte
          RTT channel 0 is always reserved for Terminal usage.
          RTT channel 1 is typically used for RTOS logs.
          RTT channel 2 is typically used for SystemView/TRACE.

---------------------------END-OF-HEADER------------------------------
*/

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>
#include <inttypes.h>
#include <getopt.h>
#include <locale.h>
#include <unistd.h>
#include <sys/stat.h>

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
#include "CLI.h"

/*********************************************************************
*
*       Defines, fixed
*
**********************************************************************
*/

#ifndef PATH_MAX
  #define PATH_MAX 4096
#endif

static char * const _TRACEHUB         = "TraceHub";
static char * const _TRACEHUB_VERSION = "0.1.0";

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

static volatile sig_atomic_t _running = 1;

/*********************************************************************
*
*       Static functions
*
**********************************************************************
*/

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

/*********************************************************************
*
*       _SwimLaneMergerCallback()
*
*  Function description
*    Callback function for sorted swimlane output.
*/
static int _SwimLaneMergerCallback(LogEntry_t *entry, void *user_data) {
    int         render_result;
    int         log_result;

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
    LogMerger_Output_t output_callback = (LogMerger_Output_t)user_data;
    int                result;

    if (output_callback == NULL) {
        LogEntry_Destroy(entry);
        return -1;
    }

    result = LogMerger_Process(entry, output_callback, NULL);
    if (result != 0) {
        Log_Error("Swimlane merger failed to process entry: %d\n", result);
        return -1;
    }

    return 0;
}

/*********************************************************************
*
*       RunConfig_t
*
*  Description
*    Runtime configuration for mode functions.
*/
typedef struct {
    unsigned linux_channel;
    unsigned rtos_channel;
    unsigned terminal_channel;
    unsigned sysview_channel;
    unsigned terminal_port;
    unsigned sysview_port;
    bool     terminal_enabled;
    bool     sysview_enabled;
    bool     core_log_enabled;
    bool     console_mode;
    bool     only_systemview;
    bool     tport_specified;
    bool     sport_specified;
    const char  *swimlane_log_prefix;
    unsigned     swimlane_flush_threshold;
    unsigned     swimlane_flush_timeout_ms;
} RunConfig_t;

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
*    0   Normal exit (user requested shutdown)
*   -1   Initialization failed
*/
static int _RunSwimLaneMode(const RunConfig_t *config) {
    int                   result;
    int                   run_result;
    int                   flush_result;
    LogCollector_Config_t collector_config;
    LogMerger_Config_t    merger_config;
    SwimLane_Config_t     swimlane_config;

    run_result = 0;

    //
    // Initialize log collector
    //
    memset(&collector_config, 0, sizeof(collector_config));
    collector_config.linux_channel    = config->linux_channel;
    collector_config.rtos_channel     = config->rtos_channel;
    collector_config.poll_interval_ms = RTT_BRIDGE_DEFAULT_POLL_INTERVAL_MS;

    result = LogCollector_Init(&collector_config);
    if (result != 0) {
        Log_Error("Failed to initialize log collector: %d\n", result);
        return -1;
    }
    //
    // Initialize log merger (log file managed internally)
    //
    memset(&merger_config, 0, sizeof(merger_config));
    merger_config.buffer_size      = LOG_MERGER_DEFAULT_BUFFER_SIZE;
    merger_config.flush_threshold  = config->swimlane_flush_threshold;
    merger_config.flush_timeout_ms = config->swimlane_flush_timeout_ms;
    merger_config.required_source[LOG_SOURCE_LINUX] = true;
    merger_config.required_source[LOG_SOURCE_RTOS]  = true;
    merger_config.log_enabled      = true;
    merger_config.log_prefix       = config->swimlane_log_prefix;

    result = LogMerger_Init(&merger_config);
    if (result != 0) {
        Log_Error("Failed to initialize log merger: %d\n", result);
        goto err_collector;
    }
    //
    // Initialize swimlane renderer
    //
    memset(&swimlane_config, 0, sizeof(swimlane_config));
    swimlane_config.timestamp_width = SWIMLANE_DEFAULT_TIMESTAMP_WIDTH;
    swimlane_config.linux_width     = SWIMLANE_DEFAULT_LINUX_WIDTH;
    swimlane_config.rtos_width      = SWIMLANE_DEFAULT_RTOS_WIDTH;
    swimlane_config.show_header     = true;
    swimlane_config.show_separator  = true;
    swimlane_config.color_enabled   = true;
    swimlane_config.output_stream   = stdout;

    result = SwimLane_Init(&swimlane_config);
    if (result != 0) {
        Log_Error("Failed to initialize swimlane renderer: %d\n", result);
        goto err_merger;
    }
    //
    // Start log collector
    //
    result = LogCollector_Start(_SwimLaneCollectorCallback, _SwimLaneMergerCallback);
    if (result != 0) {
        Log_Error("Failed to start log collector: %d\n", result);
        goto err_swimlane;
    }
    //
    // Start SystemView service if enabled
    //
    if (config->sysview_enabled) {
        result = SystemView_Start();
        if (result != 0) {
            Log_Error("Failed to start SystemView service: %d\n", result);
            goto err_collector_start;
        }
    }
    //
    // Print status
    //
    printf("*                                                                    *\r\n");
    printf("*                  Swimlane Mode Started                             *\r\n");
    printf("*                                                                    *\r\n");
    printf("*  Linux A53: channel %u                                             *\r\n", config->linux_channel);
    printf("*  RTOS R5:   channel %u                                             *\r\n", config->rtos_channel);
    if (config->sysview_enabled) {
        if (config->sport_specified) {
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
    //
    // Main loop: periodically flush merger for sorted rendering and file logging
    //
    while (_running) {
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
        SYS_Sleep((merger_config.flush_timeout_ms > 0u) ?
                  merger_config.flush_timeout_ms :
                  RTT_BRIDGE_DEFAULT_POLL_INTERVAL_MS);
        //
        // Flush buffered entries to sorted swimlane output
        //
        if (LogMerger_FlushReady(_SwimLaneMergerCallback, NULL) < 0) {
            run_result = -1;
            break;
        }
    }
    //
    // Normal cleanup
    //
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

    //
    // Error cleanup labels
    //
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
*    0   Normal exit (user requested shutdown)
*   -1   Initialization failed
*/
static int _RunNormalMode(const RunConfig_t *config) {
    int result;
    FILE *status_stream;

    //
    // Start Terminal service
    //
    result = Terminal_Start();
    if (result != 0 && config->terminal_enabled) {
        Log_Error("Failed to start Terminal service: %d\n", result);
        return -1;
    }
    //
    // Start SystemView service
    //
    result = SystemView_Start();
    if (result != 0 && config->sysview_enabled) {
        Log_Error("Failed to start SystemView service: %d\n", result);
        return -1;
    }
    //
    // Print service status
    //
    status_stream = config->console_mode ? stderr : stdout;

    fprintf(status_stream, "*                                                                    *\r\n");
    if (config->only_systemview) {
        fprintf(status_stream, "*                  SystemView Recording Mode Started                 *\r\n");
        fprintf(status_stream, "*                                                                    *\r\n");
        if (config->sport_specified) {
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
            if (config->sport_specified) {
                fprintf(status_stream, "*  SystemView: port %5u, channel %u                                 *\r\n",
                        config->sysview_port, config->sysview_channel);
            } else {
                fprintf(status_stream, "*  SystemView: local recording, channel %u                          *\r\n",
                        config->sysview_channel);
            }
        }
        fprintf(status_stream, "*                                                                    *\r\n");
        fprintf(status_stream, "*                  Press Ctrl+C to exit                              *\r\n");
    } else if (config->tport_specified) {
        fprintf(status_stream, "*                  RTT Telnet Mode Started                           *\r\n");
        fprintf(status_stream, "*                                                                    *\r\n");
        fprintf(status_stream, "*  Terminal:   port %5u, channel %u                                 *\r\n",
                config->terminal_port, config->terminal_channel);
        if (config->sysview_enabled) {
            if (config->sport_specified) {
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
            if (config->sport_specified) {
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
    //
    // Wait for termination signal
    //
    while (_running) {
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
    //
    // Normal cleanup
    //
    Log_Print("Shutting down services...\n");
    return 0;
}

/*********************************************************************
*
*       _SignalHandler()
*
*  Function description
*    Signal handler for graceful shutdown.
*/
static void _SignalHandler(int signum) {
    _running = 0;

    if (signum == SIGILL || signum == SIGFPE ||
        signum == SIGSEGV || signum == SIGABRT) {
        static const char message[] = "tracehub: fatal signal received\n";

        (void)write(STDERR_FILENO, message, sizeof(message) - 1);
        _exit(128 + signum);
    }
}

/*********************************************************************
*
*       _SignalInit()
*
*  Function description
*    Initialize signal handlers for graceful shutdown.
*/
static void _SignalInit(void) {
    struct sigaction action;

    memset(&action, 0, sizeof(action));
    action.sa_handler = _SignalHandler;
    sigemptyset(&action.sa_mask);

    if (sigaction(SIGINT, &action, NULL) != 0) {
        Log_Error("Failed to install SIGINT handler\n");
    }
    if (sigaction(SIGTERM, &action, NULL) != 0) {
        Log_Error("Failed to install SIGTERM handler\n");
    }
    if (sigaction(SIGILL, &action, NULL) != 0) {
        Log_Error("Failed to install SIGILL handler\n");
    }
    if (sigaction(SIGFPE, &action, NULL) != 0) {
        Log_Error("Failed to install SIGFPE handler\n");
    }
    if (sigaction(SIGSEGV, &action, NULL) != 0) {
        Log_Error("Failed to install SIGSEGV handler\n");
    }
    if (sigaction(SIGABRT, &action, NULL) != 0) {
        Log_Error("Failed to install SIGABRT handler\n");
    }
}

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
*       _ParseSizeAuto()
*
*  Function description
*    Parse a size value as decimal unless it uses a 0x hexadecimal prefix.
*/
static int _ParseSizeAuto(const char *str, size_t *value) {
    int base;

    if (str == NULL) {
        return -1;
    }
    base = ((str[0] == '0') && ((str[1] == 'x') || (str[1] == 'X'))) ? 16 : 10;
    return _ParseSize(str, value, base);
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
*       Command line options
*
**********************************************************************
*/

static char *_optstring = "hva:s:t:S:cd:";

static const struct option _options[] = {
    {"help",                       no_argument,       NULL, 'h'},
    {"version",                    no_argument,       NULL, 'v'},
    {"addr",                       required_argument, NULL, 'a'},
    {"size",                       required_argument, NULL, 's'},
    {"telnet-port",                required_argument, NULL, 't'},
    {"systemview-port",            required_argument, NULL, 'S'},
    {"systemview-channel",         required_argument, NULL, 1002},
    {"console",                    no_argument,       NULL, 'c'},
    {"swimlane",                   no_argument,       NULL, 1005},
    {"linux-channel",              required_argument, NULL, 1006},
    {"rtos-channel",               required_argument, NULL, 1007},
    {"rtt-timeout-ms",             required_argument, NULL, 1008},
    {"memshm-reset",               no_argument,       NULL, 1009},
    {"log-dir",                    required_argument, NULL, 1010},
    {"terminal-queue-size",        required_argument, NULL, 1011},
    {"systemview-queue-size",      required_argument, NULL, 1012},
    {"swimlane-flush-threshold",   required_argument, NULL, 1013},
    {"swimlane-flush-timeout-ms",  required_argument, NULL, 1014},
    {"core-log-queue-size",        required_argument, NULL, 1015},
    {"shm",                        required_argument, NULL, 'd'},
    {"device",                     required_argument, NULL, 'd'},
    {NULL,                         0,                 NULL,  0 }
};

/*********************************************************************
*
*       Public code
*
**********************************************************************
*/

/*********************************************************************
*
*       main
*
*/
int main(int argc, char* argv[]) {
    int  opt         =  0;
    int  lindex      = -1;
    int  result      =  0;

    // Required parameters
    char *addr_str    = NULL;
    char *size_str    = NULL;
    char *device_path = NULL;

    // Optional parameters with defaults
    unsigned terminal_port              = RTT_BRIDGE_DEFAULT_TERMINAL_PORT;
    unsigned sysview_port               = RTT_BRIDGE_DEFAULT_SYSVIEW_PORT;
    unsigned terminal_channel           = RTT_BRIDGE_DEFAULT_TERMINAL_CHANNEL;
    unsigned sysview_channel            = RTT_BRIDGE_DEFAULT_SYSVIEW_CHANNEL;
    bool     terminal_enabled           = true;
    bool     sysview_enabled            = false;
    bool     console_mode               = false;
    bool     swimlane_mode              = false;
    unsigned linux_channel              = RTT_BRIDGE_DEFAULT_TERMINAL_CHANNEL;
    unsigned rtos_channel               = RTT_BRIDGE_DEFAULT_RTOS_CHANNEL;
    size_t   terminal_queue_size        = 0u;
    size_t   sysview_queue_size         = 0u;
    size_t   core_log_queue_size        = 0u;
    unsigned swimlane_flush_threshold   = LOG_MERGER_DEFAULT_FLUSH_THRESHOLD;
    unsigned swimlane_flush_timeout_ms  = LOG_MERGER_DEFAULT_FLUSH_TIMEOUT_MS;

    // Track whether user explicitly specified options
    bool     tport_specified            = false;
    bool     sport_specified            = false;
    bool     linux_channel_specified    = false;
    bool     rtos_channel_specified     = false;
    bool     systemview_channel_specified = false;
    bool     terminal_queue_size_specified = false;
    bool     sysview_queue_size_specified = false;
    bool     core_log_queue_size_specified = false;
    bool     swimlane_flush_threshold_specified = false;
    bool     swimlane_flush_timeout_specified = false;
    bool     core_log_linux_enabled     = false;
    bool     core_log_rtos_enabled      = false;
    uint64_t rtt_address                = 0;
    size_t   rtt_region_size            = 0;
    unsigned rtt_search_timeout_ms      = 0;
    bool     memshm_reset               = false;
    char    *log_dir                    = NULL;
    char     main_log_prefix[PATH_MAX];
    char     linux_log_prefix[PATH_MAX];
    char     rtos_log_prefix[PATH_MAX];
    char     swimlane_log_prefix[PATH_MAX];
    char     sysview_log_prefix[PATH_MAX];

    // Configuration structures
    RTTBridge_Config_t        bridge_config;
    CoreLogRecorder_Config_t  core_log_config;
    Terminal_Config_t         terminal_config;
    SystemView_Config_t       sysview_config;
    bool                      core_log_fatal;
    bool                      terminal_fatal;
    bool                      sysview_fatal;
    bool                      core_log_enabled;
    bool                      terminal_service_enabled;

    if (argc <= 1) {
        printf("usage: tracehub [OPTION]. Use --help for more information.\n");
        return 0;
    }
    //
    // Set locale for correct UTF-8 display width calculation
    // This enables wcwidth() to return correct values for CJK characters
    // Must be set early at program entry before any locale-dependent operations
    //
    setlocale(LC_CTYPE, "");

    //
    // Parse command line arguments
    //
    while ((opt = getopt_long(argc, argv, _optstring, _options, &lindex)) != -1) {
        switch (opt) {
            case 'h':
                CLI_PrintUsage();
                return 0;

            case 'v':
                printf("%s %s\n", _TRACEHUB, _TRACEHUB_VERSION);
                return 0;

            case 'a':
                if (optarg == NULL) {
                    printf("--addr option requires an argument\n");
                    return 1;
                }
                addr_str = optarg;
                break;

            case 's':
                if (optarg == NULL) {
                    printf("--size option requires an argument\n");
                    return 1;
                }
                size_str = optarg;
                break;

            case 't':
                if (optarg == NULL) {
                    printf("--telnet-port option requires an argument\n");
                    return 1;
                }
                if (_ParsePort(optarg, &terminal_port) != 0) {
                    printf("Error: invalid --telnet-port value: %s\n", optarg);
                    return 1;
                }
                tport_specified = true;
                break;

            case 'S':
                if (optarg == NULL) {
                    printf("--systemview-port option requires an argument\n");
                    return 1;
                }
                if (_ParsePort(optarg, &sysview_port) != 0) {
                    printf("Error: invalid --systemview-port value: %s\n", optarg);
                    return 1;
                }
                sport_specified = true;
                break;

            case 1002:  // --systemview-channel
                if (optarg == NULL) {
                    printf("--systemview-channel option requires an argument\n");
                    return 1;
                }
                if (_ParseUnsigned(optarg, &sysview_channel, 10) != 0) {
                    printf("Error: invalid --systemview-channel value: %s\n", optarg);
                    return 1;
                }
                systemview_channel_specified = true;
                break;

            case 'c':  // --console
                console_mode = true;
                break;

            case 1005:  // --swimlane
                swimlane_mode = true;
                break;

            case 1006:  // --linux-channel
                if (optarg == NULL) {
                    printf("--linux-channel option requires an argument\n");
                    return 1;
                }
                if (_ParseUnsigned(optarg, &linux_channel, 10) != 0) {
                    printf("Error: invalid --linux-channel value: %s\n", optarg);
                    return 1;
                }
                linux_channel_specified = true;
                break;

            case 1007:  // --rtos-channel
                if (optarg == NULL) {
                    printf("--rtos-channel option requires an argument\n");
                    return 1;
                }
                if (_ParseUnsigned(optarg, &rtos_channel, 10) != 0) {
                    printf("Error: invalid --rtos-channel value: %s\n", optarg);
                    return 1;
                }
                rtos_channel_specified = true;
                break;

            case 1008:  // --rtt-timeout-ms
                if (optarg == NULL) {
                    printf("--rtt-timeout-ms option requires an argument\n");
                    return 1;
                }
                if (_ParseUnsigned(optarg, &rtt_search_timeout_ms, 10) != 0) {
                    printf("Error: invalid --rtt-timeout-ms value: %s\n", optarg);
                    return 1;
                }
                break;

            case 1009:  // --memshm-reset
#if defined(RTTMEM_USE_MEMSHM)
                memshm_reset = true;
#else
                printf("Error: --memshm-reset requires a MEMSHM backend build.\n");
                return 1;
#endif
                break;

            case 1010:  // --log-dir
                if ((optarg == NULL) || (optarg[0] == '\0')) {
                    printf("--log-dir option requires a non-empty argument\n");
                    return 1;
                }
                log_dir = optarg;
                break;

            case 1011:  // --terminal-queue-size
                if (optarg == NULL) {
                    printf("--terminal-queue-size option requires an argument\n");
                    return 1;
                }
                if (_ParseSizeAuto(optarg, &terminal_queue_size) != 0) {
                    printf("Error: invalid --terminal-queue-size value: %s\n", optarg);
                    return 1;
                }
                terminal_queue_size_specified = true;
                break;

            case 1012:  // --systemview-queue-size
                if (optarg == NULL) {
                    printf("--systemview-queue-size option requires an argument\n");
                    return 1;
                }
                if (_ParseSizeAuto(optarg, &sysview_queue_size) != 0) {
                    printf("Error: invalid --systemview-queue-size value: %s\n", optarg);
                    return 1;
                }
                sysview_queue_size_specified = true;
                break;

            case 1013:  // --swimlane-flush-threshold
                if (optarg == NULL) {
                    printf("--swimlane-flush-threshold option requires an argument\n");
                    return 1;
                }
                if ((_ParseUnsigned(optarg, &swimlane_flush_threshold, 10) != 0) ||
                    (swimlane_flush_threshold == 0u)) {
                    printf("Error: invalid --swimlane-flush-threshold value: %s\n", optarg);
                    return 1;
                }
                swimlane_flush_threshold_specified = true;
                break;

            case 1014:  // --swimlane-flush-timeout-ms
                if (optarg == NULL) {
                    printf("--swimlane-flush-timeout-ms option requires an argument\n");
                    return 1;
                }
                if (_ParseUnsigned(optarg, &swimlane_flush_timeout_ms, 10) != 0) {
                    printf("Error: invalid --swimlane-flush-timeout-ms value: %s\n", optarg);
                    return 1;
                }
                swimlane_flush_timeout_specified = true;
                break;

            case 1015:  // --core-log-queue-size
                if (optarg == NULL) {
                    printf("--core-log-queue-size option requires an argument\n");
                    return 1;
                }
                if (_ParseSizeAuto(optarg, &core_log_queue_size) != 0) {
                    printf("Error: invalid --core-log-queue-size value: %s\n", optarg);
                    return 1;
                }
                if ((core_log_queue_size > 0u) &&
                    (core_log_queue_size < CORE_LOG_RECORDER_MIN_QUEUE_SIZE)) {
                    printf("Error: --core-log-queue-size must be 0 or >= %u.\n",
                           (unsigned)CORE_LOG_RECORDER_MIN_QUEUE_SIZE);
                    return 1;
                }
                core_log_queue_size_specified = true;
                break;

            case 'd':  // --shm or --device
                if (optarg == NULL) {
                    printf("--shm/--device option requires an argument\n");
                    return 1;
                }
                device_path = optarg;
                break;

            default:
                printf("Invalid option.\n");
                CLI_PrintUsage();
                return 1;
        }
    }

    //
    // Validate required parameters
    //
    if (addr_str == NULL || size_str == NULL) {
        printf("--addr and --size parameters are required.\n");
        CLI_PrintUsage();
        return 1;
    }
    if (device_path == NULL) {
        printf("--shm or --device parameter is required.\n");
        CLI_PrintUsage();
        return 1;
    }
    if (_ParseU64(addr_str, &rtt_address, 16) != 0) {
        printf("Error: invalid --addr value: %s\n", addr_str);
        return 1;
    }
    if (_ParseSize(size_str, &rtt_region_size, 16) != 0) {
        printf("Error: invalid --size value: %s\n", size_str);
        return 1;
    }

    //
    // Validate and determine service enablement
    //

    if (console_mode && swimlane_mode) {
        printf("Error: --console and --swimlane are mutually exclusive.\n");
        return 1;
    }

    // Validate console mode
    if (console_mode) {
        if (linux_channel_specified && rtos_channel_specified) {
            printf("Error: --console cannot use both --linux-channel and --rtos-channel.\n");
            printf("Use --swimlane for dual-channel display.\n");
            return 1;
        }
        if (tport_specified) {
            printf("Error: --console and --telnet-port are mutually exclusive.\n");
            return 1;
        }
    }

    // Validate swimlane mode
    if (swimlane_mode) {
        if (tport_specified) {
            printf("Error: --swimlane and --telnet-port are mutually exclusive.\n");
            return 1;
        }
    }

    // Validate Telnet mode
    if (tport_specified) {
        if (linux_channel_specified && rtos_channel_specified) {
            printf("Error: Cannot specify both --linux-channel and --rtos-channel with --telnet-port.\n");
            printf("Telnet mode supports single channel only. Use --swimlane for dual-channel display.\n");
            return 1;
        }
    }

    // Determine SystemView enablement
    sysview_enabled = systemview_channel_specified || sport_specified;

    // Determine if user only wants SystemView (no Terminal)
    bool only_systemview = sysview_enabled &&
                           !console_mode &&
                           !swimlane_mode &&
                           !tport_specified &&
                           !linux_channel_specified &&
                           !rtos_channel_specified;
    // Determine Terminal enablement and mode
    if (only_systemview) {
        //
        // Only SystemView mode: disable Terminal
        //
        terminal_enabled = false;
    } else if (tport_specified) {
        //
        // Telnet mode: Terminal service via network, no terminal display
        //
        terminal_enabled = true;
        console_mode     = false;
        swimlane_mode    = false;

        // Determine which channel Terminal should use
        if (linux_channel_specified) {
            terminal_channel = linux_channel;
            core_log_linux_enabled = true;
        } else if (rtos_channel_specified) {
            terminal_channel = rtos_channel;
            core_log_rtos_enabled = true;
        } else {
            // Default: use linux-channel
            terminal_channel = linux_channel;
            core_log_linux_enabled = true;
        }
    } else if (console_mode) {
        //
        // Console mode: single channel on terminal
        //
        terminal_enabled = true;
        swimlane_mode    = false;

        // Determine which channel to use
        if (linux_channel_specified) {
            terminal_channel = linux_channel;
            core_log_linux_enabled = true;
        } else if (rtos_channel_specified) {
            terminal_channel = rtos_channel;
            core_log_rtos_enabled = true;
        } else {
            // Default: use linux-channel
            terminal_channel = linux_channel;
            core_log_linux_enabled = true;
        }
    } else if (swimlane_mode) {
        //
        // Swimlane mode: dual channels on terminal
        //
        terminal_enabled = true;
        // Use specified channels or defaults
        // (linux_channel and rtos_channel already have default values)
        core_log_linux_enabled = true;
        core_log_rtos_enabled  = true;
    } else {
        //
        // Default mode: Swimlane mode with default channels
        //
        terminal_enabled = true;
        console_mode     = false;
        swimlane_mode    = true;
        // Use specified channels or defaults
        // (linux_channel and rtos_channel already have default values)
        core_log_linux_enabled = true;
        core_log_rtos_enabled  = true;
    }

    core_log_enabled = core_log_linux_enabled || core_log_rtos_enabled;
    terminal_service_enabled = terminal_enabled && !swimlane_mode;

    if (!swimlane_mode &&
        (swimlane_flush_threshold_specified || swimlane_flush_timeout_specified)) {
        printf("Error: --swimlane-flush-threshold and --swimlane-flush-timeout-ms require swimlane mode.\n");
        return 1;
    }
    if (swimlane_mode &&
        swimlane_flush_threshold > (unsigned)LOG_MERGER_DEFAULT_BUFFER_SIZE) {
        printf("Error: --swimlane-flush-threshold must be <= %u.\n",
               (unsigned)LOG_MERGER_DEFAULT_BUFFER_SIZE);
        return 1;
    }
    if (!tport_specified && terminal_queue_size_specified) {
        printf("Error: --terminal-queue-size requires --telnet-port.\n");
        return 1;
    }
    if (!sport_specified && sysview_queue_size_specified) {
        printf("Error: --systemview-queue-size requires --systemview-port.\n");
        return 1;
    }
    if (!core_log_enabled && core_log_queue_size_specified) {
        printf("Error: --core-log-queue-size requires an enabled core log source.\n");
        return 1;
    }

    if (!terminal_enabled && !sysview_enabled) {
        printf("Error: No service enabled.\n");
        return 1;
    }

    //
    // Validate channel conflicts
    //
    if (core_log_linux_enabled && core_log_rtos_enabled && linux_channel == rtos_channel) {
        printf("Error: Linux channel (%u) conflicts with RTOS channel (%u).\n",
               linux_channel, rtos_channel);
        printf("Core log recording requires two different RTT channels.\n");
        return 1;
    }

    if (sysview_enabled && core_log_enabled) {
        if (core_log_linux_enabled && sysview_channel == linux_channel) {
            printf("Error: SystemView channel (%u) conflicts with Linux log channel (%u).\n",
                   sysview_channel, linux_channel);
            printf("Each enabled service must use a different RTT channel.\n");
            return 1;
        }
        if (core_log_rtos_enabled && sysview_channel == rtos_channel) {
            printf("Error: SystemView channel (%u) conflicts with RTOS log channel (%u).\n",
                   sysview_channel, rtos_channel);
            printf("Each enabled service must use a different RTT channel.\n");
            return 1;
        }
        if (!swimlane_mode && terminal_enabled) {
            //
            // In console/telnet mode, check SystemView channel doesn't conflict with Terminal channel
            //
            if (sysview_channel == terminal_channel) {
                printf("Error: SystemView channel (%u) conflicts with Terminal channel (%u).\n",
                       sysview_channel, terminal_channel);
                printf("Each service must use a different RTT channel.\n");
                return 1;
            }
        }
    } else if (sysview_enabled && terminal_enabled) {
        if (sysview_channel == terminal_channel) {
            printf("Error: SystemView channel (%u) conflicts with Terminal channel (%u).\n",
                   sysview_channel, terminal_channel);
            printf("Each service must use a different RTT channel.\n");
            return 1;
        }
    }

    if (_ValidateLogDir(log_dir) != 0) {
        return 1;
    }

    if ((_BuildLogPrefix(main_log_prefix, sizeof(main_log_prefix), log_dir, "main") != 0) ||
        (_BuildLogPrefix(linux_log_prefix, sizeof(linux_log_prefix), log_dir, "linux") != 0) ||
        (_BuildLogPrefix(rtos_log_prefix, sizeof(rtos_log_prefix), log_dir, "rtos") != 0) ||
        (_BuildLogPrefix(swimlane_log_prefix, sizeof(swimlane_log_prefix), log_dir, "swimlane") != 0) ||
        (_BuildLogPrefix(sysview_log_prefix, sizeof(sysview_log_prefix), log_dir, "sysview") != 0)) {
        printf("Error: log path prefix is too long.\n");
        return 1;
    }

    CLI_PrintBanner(console_mode ? stderr : stdout);

    //
    // Initialize main log file
    //
    result = LOG_InitEx(main_log_prefix);
    if (result != 0) {
        fprintf(stderr, "Warning: Failed to initialize main log file: %d\n", result);
    }

    //
    // Initialize signal handlers
    //
    _SignalInit();

    //
    // Initialize RTT Bridge
    //
    memset(&bridge_config, 0, sizeof(bridge_config));
    bridge_config.rtt_address      = rtt_address;
    bridge_config.rtt_region_size  = rtt_region_size;
    bridge_config.poll_interval_ms = RTT_BRIDGE_DEFAULT_POLL_INTERVAL_MS;
    bridge_config.device_path      = device_path;
    bridge_config.log_file         = NULL;
    bridge_config.log_file_handle  = NULL;
    bridge_config.run_flag         = &_running;
    bridge_config.rtt_search_timeout_ms = rtt_search_timeout_ms;
    bridge_config.reset_memory     = memshm_reset;
    bridge_config.debug            = false;

    result = RTTBridge_Init(&bridge_config);
    if (result != 0) {
        Log_Error("Failed to initialize RTT bridge: %d\n", result);
        goto err_rttbridge;
    }

    //
    // Initialize Linux and RTOS core log recording for modes that consume core logs.
    //
    if (core_log_enabled) {
        memset(&core_log_config, 0, sizeof(core_log_config));
        core_log_config.linux_channel        = linux_channel;
        core_log_config.rtos_channel         = rtos_channel;
        core_log_config.poll_interval_ms     = RTT_BRIDGE_DEFAULT_POLL_INTERVAL_MS;
        core_log_config.consumer_queue_size  = core_log_queue_size;
        core_log_config.linux_prefix         = linux_log_prefix;
        core_log_config.rtos_prefix          = rtos_log_prefix;
        core_log_config.linux_enabled        = core_log_linux_enabled;
        core_log_config.rtos_enabled         = core_log_rtos_enabled;

        result = CoreLogRecorder_Init(&core_log_config);
        if (result != 0) {
            Log_Error("Failed to initialize core log recorder: %d\n", result);
            goto err_corelog;
        }
    }

    //
    // Initialize Terminal service. Core log files are owned by CoreLogRecorder.
    //
    memset(&terminal_config, 0, sizeof(terminal_config));
    terminal_config.port         = terminal_port;
    terminal_config.channel      = terminal_channel;
    terminal_config.enabled      = terminal_service_enabled;
    terminal_config.console_mode = console_mode;
    terminal_config.log_enabled  = false;
    terminal_config.log_prefix   = NULL;
    terminal_config.network_queue_size = terminal_queue_size;

    result = Terminal_Init(&terminal_config);
    if (result != 0) {
        Log_Error("Failed to initialize Terminal service: %d\n", result);
        goto err_terminal;
    }

    //
    // Initialize SystemView service (record file managed internally)
    //
    memset(&sysview_config, 0, sizeof(sysview_config));
    sysview_config.port            = sysview_port;
    sysview_config.channel         = sysview_channel;
    sysview_config.enabled         = sysview_enabled;
    sysview_config.network_enabled = sport_specified;
    sysview_config.record_enabled  = sysview_enabled;
    sysview_config.record_prefix   = sysview_log_prefix;
    sysview_config.network_queue_size = sysview_queue_size;

    result = SystemView_Init(&sysview_config);
    if (result != 0) {
        Log_Error("Failed to initialize SystemView service: %d\n", result);
        goto err_sysview;
    }

    //
    // Start services
    //
    RTTBridge_SetRunning(true);
    if (core_log_enabled) {
        result = CoreLogRecorder_Start();
        if (result != 0) {
            Log_Error("Failed to start core log recorder: %d\n", result);
            goto err_sysview;
        }
    }

    //
    // Prepare runtime configuration
    //
    RunConfig_t run_config;
    memset(&run_config, 0, sizeof(run_config));
    run_config.linux_channel     = linux_channel;
    run_config.rtos_channel      = rtos_channel;
    run_config.terminal_channel  = terminal_channel;
    run_config.sysview_channel   = sysview_channel;
    run_config.terminal_port     = terminal_port;
    run_config.sysview_port      = sysview_port;
    run_config.terminal_enabled  = terminal_service_enabled;
    run_config.sysview_enabled   = sysview_enabled;
    run_config.core_log_enabled  = core_log_enabled;
    run_config.console_mode      = console_mode;
    run_config.only_systemview   = only_systemview;
    run_config.tport_specified   = tport_specified;
    run_config.sport_specified   = sport_specified;
    run_config.swimlane_log_prefix = swimlane_log_prefix;
    run_config.swimlane_flush_threshold = swimlane_flush_threshold;
    run_config.swimlane_flush_timeout_ms = swimlane_flush_timeout_ms;

    //
    // Run mode-specific logic
    //
    if (swimlane_mode) {
        result = _RunSwimLaneMode(&run_config);
    } else {
        result = _RunNormalMode(&run_config);
    }

    if (result != 0) {
        goto err_sysview;
    }

    //
    // Common cleanup (normal exit)
    //
    SystemView_Stop();
    Terminal_Stop();
    terminal_fatal = Terminal_HasFatalError();
    if (core_log_enabled) {
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

    //
    // Error cleanup labels (reverse initialization order)
    //
err_sysview:
    SystemView_Stop();
err_terminal:
    Terminal_Stop();
    if (core_log_enabled) {
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
