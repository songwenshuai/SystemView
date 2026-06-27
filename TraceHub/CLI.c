/*********************************************************************
*                                                                    *
*                Copyright (C) 2023 xrTest Inc.                      *
*                      All rights reserved                           *
*                                                                    *
**********************************************************************
----------------------------------------------------------------------
File    : CLI.c
Purpose : Command line banner and usage output
Author  : songwenshuai <songwenshuai@gmail.com>
---------------------------END-OF-HEADER------------------------------
*/

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include "CLI.h"
#include "CoreLogRecorder.h"
#include "LogMerger.h"
#include "RTTBridge.h"

/*********************************************************************
*
*       Public functions
*
**********************************************************************
*/

/*********************************************************************
*
*       CLI_PrintBanner()
*/
void CLI_PrintBanner(FILE *stream) {
    if (stream == NULL) {
        stream = stdout;
    }

    fprintf(stream, "/*********************************************************************\r\n");
    fprintf(stream, "*                                                                    *\r\n");
    fprintf(stream, "*                Copyright (C) 2023 xrTest Inc.                      *\r\n");
    fprintf(stream, "*                      All rights reserved                           *\r\n");
    fprintf(stream, "*                                                                    *\r\n");
    fprintf(stream, "*            TraceHub Compiled " __DATE__ " " __TIME__ "                  *\r\n");
    fprintf(stream, "*                                                                    *\r\n");
    fprintf(stream, "*        Author: songwenshuai <songwenshuai@gmail.com>             *\r\n");
    fprintf(stream, "*                                                                    *\r\n");
    fprintf(stream, "**********************************************************************\r\n");
}

/*********************************************************************
*
*       CLI_PrintUsage()
*/
void CLI_PrintUsage(void) {
    printf("Usage:\n");
    printf("  tracehub --addr <addr> --size <size> --shm <name> [OPTIONS]\n");
    printf("\n");
    printf("Global options:\n");
    printf("  --help, -h            Show this help message\n");
    printf("  --version, -v         Show version information\n");
    printf("\n");
    printf("Required parameters:\n");
    printf("====================\n");
    printf("--addr, -a <hex_address>\n");
    printf("  Backend start address of RTT region to map, search, and validate (hex format).\n");
    printf("  Use 0 to select the backend base address.\n");
    printf("  Example: --addr 0x10000000\n");
    printf("\n");
    printf("--size, -s <hex_size>\n");
    printf("  RTT region size to map, search, and validate (hex format).\n");
    printf("  Use 0 to select the backend mapped size.\n");
    printf("  Example: --size 0\n");
    printf("\n");
    printf("--shm, --device, -d <path>\n");
    printf("  Device path or shared memory name for RTT memory access.\n");
    printf("  For MEMSHM backend: POSIX shared memory name (e.g., /rtt_sim)\n");
    printf("  For SMEM backend: SharedMem device path (e.g., /dev/shared_mem0)\n");
    printf("  Example: --shm /rtt_sim\n");
    printf("\n");
    printf("--rtt-timeout-ms <ms>\n");
    printf("  Maximum time to wait for RTT control block discovery, in milliseconds.\n");
    printf("  Use 0 to wait until interrupted (default: 0).\n");
    printf("  Example: --rtt-timeout-ms 30000\n");
    printf("\n");
    printf("--memshm-reset\n");
    printf("  Reset the POSIX shared memory object before MEMSHM mapping.\n");
    printf("  This option is only valid for MEMSHM simulation builds.\n");
    printf("  Do not use it while simulator processes keep running on the same object.\n");
    printf("  Restart all simulator processes after resetting shared memory.\n");
    printf("\n");
    printf("--log-dir <dir>\n");
    printf("  Directory for all timestamped log and record files.\n");
    printf("  The directory must already exist. Default: current working directory.\n");
    printf("  Example: --log-dir ./logs\n");
    printf("\n");
    printf("Optional parameters:\n");
    printf("====================\n");
    printf("--linux-channel <n>\n");
    printf("  RTT channel for Linux logs (default: %u).\n", RTT_BRIDGE_DEFAULT_TERMINAL_CHANNEL);
    printf("  Used by Terminal service to display Linux A53 logs.\n");
    printf("  Optional: uses default if not specified.\n");
    printf("  Example: --linux-channel 0\n");
    printf("\n");
    printf("--rtos-channel <n>\n");
    printf("  RTT channel for RTOS logs (default: %u).\n", RTT_BRIDGE_DEFAULT_RTOS_CHANNEL);
    printf("  Used by Terminal service to display RTOS R5 logs.\n");
    printf("  Optional: uses default if not specified.\n");
    printf("  Example: --rtos-channel 1\n");
    printf("\n");
    printf("--systemview-channel <n>\n");
    printf("  RTT channel for SystemView binary trace data (default: %u).\n", RTT_BRIDGE_DEFAULT_SYSVIEW_CHANNEL);
    printf("  Must not match any enabled core log channel.\n");
    printf("  If specified alone: local recording only (no Terminal).\n");
    printf("  If specified with Terminal options: both services run.\n");
    printf("  Example: --systemview-channel 2\n");
    printf("\n");
    printf("--console, -c\n");
    printf("  Console mode: display single channel on terminal (stdin/stdout).\n");
    printf("  Optional: if no channel specified, uses --linux-channel 0 by default.\n");
    printf("  Can specify --linux-channel or --rtos-channel to override.\n");
    printf("  Example: --console (uses default), or --console --rtos-channel 1\n");
    printf("\n");
    printf("--swimlane\n");
    printf("  Swimlane mode: display dual channels on terminal in parallel columns.\n");
    printf("  Optional: if channels not specified, uses defaults (linux=0, rtos=1).\n");
    printf("  Can specify channels explicitly to override defaults.\n");
    printf("  Example: --swimlane (uses defaults), or --swimlane --linux-channel 0 --rtos-channel 1\n");
    printf("\n");
    printf("--telnet-port, -t <port>\n");
    printf("  Enable Terminal Telnet service on specified TCP port (default: %u).\n", RTT_BRIDGE_DEFAULT_TERMINAL_PORT);
    printf("  When enabled, Terminal output goes to Telnet instead of terminal.\n");
    printf("  Example: --telnet-port 2323\n");
    printf("\n");
    printf("--systemview-port, -S <port>\n");
    printf("  Enable SystemView network service on specified TCP port (default: %u).\n", RTT_BRIDGE_DEFAULT_SYSVIEW_PORT);
    printf("  Uses default SystemView channel if --systemview-channel is not specified.\n");
    printf("  Example: --systemview-port 19111\n");
    printf("\n");
    printf("--terminal-queue-size <bytes>\n");
    printf("  Terminal TCP backlog size in bytes. Requires --telnet-port. Use 0 for the default 1 MiB.\n");
    printf("  Example: --telnet-port 2323 --terminal-queue-size 4194304\n");
    printf("\n");
    printf("--systemview-queue-size <bytes>\n");
    printf("  SystemView TCP backlog size in bytes. Requires --systemview-port. Use 0 for the default 1 MiB.\n");
    printf("  Example: --systemview-port 19111 --systemview-queue-size 0x400000\n");
    printf("\n");
    printf("--swimlane-flush-threshold <count>\n");
    printf("  Swimlane ready flush threshold. Default: %u entries.\n",
           (unsigned)LOG_MERGER_DEFAULT_FLUSH_THRESHOLD);
    printf("  Must be between 1 and %u.\n", (unsigned)LOG_MERGER_DEFAULT_BUFFER_SIZE);
    printf("  Example: --swimlane-flush-threshold 16\n");
    printf("\n");
    printf("--swimlane-flush-timeout-ms <ms>\n");
    printf("  Swimlane ready flush timeout in milliseconds. Default: %u.\n",
           (unsigned)LOG_MERGER_DEFAULT_FLUSH_TIMEOUT_MS);
    printf("  Use 0 to disable timeout flushing and wait for source watermarks.\n");
    printf("  Example: --swimlane-flush-timeout-ms 500\n");
    printf("\n");
    printf("--core-log-queue-size <bytes>\n");
    printf("  Per-source core log consumer queue size. Use 0 for the default %u bytes.\n",
           (unsigned)CORE_LOG_RECORDER_DEFAULT_QUEUE_SIZE);
    printf("  Non-zero values must be at least %u bytes.\n",
           (unsigned)CORE_LOG_RECORDER_MIN_QUEUE_SIZE);
    printf("  Example: --core-log-queue-size 0x400000\n");
    printf("\n");
    printf("Examples:\n");
    printf("  # 1. Default: Swimlane mode with default channels (linux=0, rtos=1)\n");
    printf("  tracehub --addr 0x10000000 --size 0 --shm /rtt_sim\n");
    printf("\n");
    printf("  # 2. Swimlane mode (explicitly, same as default)\n");
    printf("  tracehub --addr 0x10000000 --size 0 --shm /rtt_sim --swimlane\n");
    printf("\n");
    printf("  # 3. Swimlane mode with custom channels\n");
    printf("  tracehub --addr 0x10000000 --size 0 --shm /rtt_sim --swimlane --linux-channel 0 --rtos-channel 1\n");
    printf("\n");
    printf("  # 4. Console mode with default channel (linux=0)\n");
    printf("  tracehub --addr 0x10000000 --size 0 --shm /rtt_sim --console\n");
    printf("\n");
    printf("  # 5. Console mode with specific channel (RTOS)\n");
    printf("  tracehub --addr 0x10000000 --size 0 --shm /rtt_sim --console --rtos-channel 1\n");
    printf("\n");
    printf("  # 6. Telnet service with default channel (linux=0)\n");
    printf("  tracehub --addr 0x10000000 --size 0 --shm /rtt_sim --telnet-port 2323\n");
    printf("\n");
    printf("  # 7. Telnet service with custom channel (RTOS)\n");
    printf("  tracehub --addr 0x10000000 --size 0 --shm /rtt_sim --telnet-port 2323 --rtos-channel 1\n");
    printf("\n");
    printf("  # 8. Only SystemView local recording (no Terminal)\n");
    printf("  tracehub --addr 0x10000000 --size 0 --shm /rtt_sim --systemview-channel 2\n");
    printf("\n");
    printf("  # 9. SystemView network service with local SVDat recording (no Terminal)\n");
    printf("  tracehub --addr 0x10000000 --size 0 --shm /rtt_sim --systemview-port 19111\n");
    printf("\n");
    printf("  # 10. Swimlane + SystemView local recording\n");
    printf("  tracehub --addr 0x10000000 --size 0 --shm /rtt_sim --swimlane --systemview-channel 2\n");
    printf("\n");
    printf("  # 11. Swimlane + SystemView network and local SVDat recording\n");
    printf("  tracehub --addr 0x10000000 --size 0 --shm /rtt_sim --swimlane --systemview-port 19111\n");
    printf("\n");
    printf("  # 12. Specify channel to enable Terminal with SystemView\n");
    printf("  tracehub --addr 0x10000000 --size 0 --shm /rtt_sim --linux-channel 0 --systemview-channel 2\n");
    printf("         (Enables Swimlane with linux=0, rtos=1 (default) + SystemView)\n");
    printf("\n");
    printf("Note:\n");
    printf("  - The SMEM backend may require root privileges to access mapped memory.\n");
    printf("  - Data is automatically logged to timestamped files according to enabled services:\n");
    printf("  - Use --log-dir to choose the directory for generated files.\n");
    printf("    * main_<timestamp>.log      - Main tool log\n");
    printf("    * linux_<timestamp>.log     - Linux core log data when Linux source is enabled\n");
    printf("    * rtos_<timestamp>.log      - RTOS core log data when RTOS source is enabled\n");
    printf("    * sysview_<timestamp>.SVDat - SystemView binary SVDat data (configured RTT channel)\n");
    printf("    * swimlane_<timestamp>.log  - Swimlane timestamped log (swimlane mode only)\n");
    printf("\n");
    printf("Author: songwenshuai <songwenshuai@gmail.com>\n");
    printf("\n");
}

/*************************** End of file ****************************/
