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
File    : CLI.c
Purpose : Command line banner and usage output
---------------------------END-OF-HEADER------------------------------
*/

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include "CLI.h"
#include "TraceHubDefaults.h"
#include "RTTBridge.h"
#include "SwimLaneRenderer.h"

/*********************************************************************
*
*       Public functions
*
**********************************************************************
*/

/*********************************************************************
*
*       CLI_PrintBanner()
*
*  Function description
*    Print the TraceHub banner to the specified stream.
*
*  Parameters
*    stream  Output stream, defaults to stdout when NULL.
*/
void CLI_PrintBanner(FILE *stream) {
    if (stream == NULL) {
        stream = stdout;
    }

    fprintf(stream, "/*********************************************************************\r\n");
    fprintf(stream, "*                             CineLogic                              *\r\n");
    fprintf(stream, "*                      RTT Trace and Debug Bridge                    *\r\n");
    fprintf(stream, "*                    (c) 2023 - 2026 CineLogic                       *\r\n");
    fprintf(stream, "*                                                                    *\r\n");
    fprintf(stream, "*            TraceHub Compiled " __DATE__ " " __TIME__ "                  *\r\n");
    fprintf(stream, "*                                                                    *\r\n");
    fprintf(stream, "*                  Support: wenshuaisong@gmail.com                   *\r\n");
    fprintf(stream, "*                                                                    *\r\n");
    fprintf(stream, "**********************************************************************\r\n");
}

/*********************************************************************
*
*       CLI_PrintUsage()
*
*  Function description
*    Print command-line usage information to stdout.
*/
void CLI_PrintUsage(void) {
    printf("TraceHub - RTT Trace and Debug Bridge\n");
    printf("\n");
    printf("Usage:\n");
    printf("  tracehub [RTT_MEMORY] [MODE] [SYSTEMVIEW] [OUTPUT]\n");
    printf("  tracehub --help\n");
    printf("  tracehub --version\n");
    printf("\n");
    printf("No runtime argument is mandatory. Build-time defaults are used when an\n");
    printf("option is omitted. RTT memory binding options are kept as runtime\n");
    printf("overrides so one binary can attach to different RTT regions.\n");
    printf("\n");
    printf("Global options:\n");
    printf("  -h, --help\n");
    printf("      Show this help message.\n");
    printf("\n");
    printf("  -v, --version\n");
    printf("      Show version information.\n");
    printf("\n");
    printf("RTT memory binding:\n");
    printf("  -a, --addr <addr>\n");
    printf("      RTT region search base address. Decimal and 0x-prefixed values are accepted.\n");
    printf("      Use 0 to select the backend base address. Default: 0x%llx.\n",
           (unsigned long long)TRACEHUB_DEFAULT_RTT_ADDR);
    printf("      Example: --addr 0x10000000\n");
    printf("\n");
    printf("  -s, --size <size>\n");
    printf("      RTT region search size. Decimal and 0x-prefixed values are accepted.\n");
    printf("      Use 0 to select the backend mapped size. Default: 0x%llx.\n",
           (unsigned long long)TRACEHUB_DEFAULT_RTT_SIZE);
    printf("      Example: --size 0\n");
    printf("\n");
    printf("  --shm <name>\n");
    printf("      Backend access path or shared memory name. Default: %s.\n",
           TRACEHUB_DEFAULT_MEMORY_PATH);
    printf("      MEMSHM backend: host shared memory name, for example /rtt_sim.\n");
    printf("      SMEM backend: Linux SharedMem device path, for example /dev/shared_mem0.\n");
    printf("      Example: --shm /rtt_sim\n");
    printf("\n");
    printf("Mode selection:\n");
    printf("  -c, --console\n");
    printf("      Single-source console mode using stdin/stdout.\n");
    printf("      Uses the build-time Terminal channel unless a Linux or RTOS source option is specified.\n");
    printf("\n");
    printf("  --linux\n");
    printf("      Select Linux text logs for console or Telnet single-source mode.\n");
    printf("\n");
    printf("  --rtos\n");
    printf("      Select RTOS text logs for console or Telnet single-source mode.\n");
    printf("\n");
    printf("  --linux-channel <n>\n");
    printf("      RTT text channel for Linux logs. Default: %u.\n",
           RTT_BRIDGE_DEFAULT_LINUX_CHANNEL);
    printf("      In console or Telnet mode, this also selects Linux as the single source.\n");
    printf("      Example: --linux-channel 0\n");
    printf("\n");
    printf("  --rtos-channel <n>\n");
    printf("      RTT text channel for RTOS logs. Default: %u.\n",
           RTT_BRIDGE_DEFAULT_RTOS_CHANNEL);
    printf("      In console or Telnet mode, this also selects RTOS as the single source.\n");
    printf("      Example: --rtos-channel 1\n");
    printf("\n");
    printf("  --swimlane\n");
    printf("      Dual-source Linux/RTOS swimlane mode. This is the default mode.\n");
    printf("      Both sources are shown in timestamp order using parallel terminal columns.\n");
    printf("\n");
    printf("  -t, --telnet-port <port>\n");
    printf("      Enable single-source Terminal Telnet service on the specified TCP port.\n");
    printf("      Uses the build-time Terminal channel unless a Linux or RTOS source option is specified.\n");
    printf("      Default Terminal port: %u. Example: --telnet-port 2323\n",
           RTT_BRIDGE_DEFAULT_TERMINAL_PORT);
    printf("\n");
    printf("SystemView:\n");
    printf("  --systemview\n");
    printf("      Enable local SystemView SVDat recording.\n");
    printf("\n");
    printf("  --systemview-channel <n>\n");
    printf("      RTT channel for SystemView binary trace data. Default: %u.\n",
           RTT_BRIDGE_DEFAULT_SYSVIEW_CHANNEL);
    printf("      Also enables local SystemView SVDat recording.\n");
    printf("      Must not match any enabled Linux or RTOS text log channel.\n");
    printf("      Example: --systemview-channel 2\n");
    printf("\n");
    printf("  -S, --systemview-port <port>\n");
    printf("      Enable SystemView TCP service and local SVDat recording.\n");
    printf("      Default SystemView port: %u. Example: --systemview-port 19111\n",
           RTT_BRIDGE_DEFAULT_SYSVIEW_PORT);
    printf("\n");
    printf("Output and runtime control:\n");
    printf("  --log-dir <dir>\n");
    printf("      Directory for timestamped log and record files. The directory must\n");
    printf("      already exist. Default: current working directory.\n");
    printf("      Example: --log-dir ./logs\n");
    printf("\n");
    printf("  --rtt-timeout-ms <ms>\n");
    printf("      Maximum time to wait for RTT control block discovery, in milliseconds.\n");
    printf("      Use 0 to wait until interrupted. Default: 0.\n");
    printf("      Example: --rtt-timeout-ms 30000\n");
    printf("\n");
    printf("  --swimlane-width <columns>\n");
    printf("      Override the swimlane total terminal width. Use 0 for automatic width resolution.\n");
    printf("      Requires swimlane mode; no explicit mode also uses swimlane.\n");
    printf("      Default non-TTY width: %u. Example: --swimlane-width 160\n",
           (unsigned)SWIMLANE_DEFAULT_TOTAL_WIDTH);
#if defined(RTTMEM_USE_MEMSHM)
    printf("\n");
    printf("  --memshm-reset\n");
    printf("      Reset the MEMSHM object before mapping it. Use only for host simulation.\n");
    printf("      Do not use while simulator processes are still attached to the same object.\n");
#endif
    printf("\n");
    printf("Build-time defaults:\n");
    printf("  RTT address:               0x%llx\n",
           (unsigned long long)TRACEHUB_DEFAULT_RTT_ADDR);
    printf("  RTT size:                  0x%llx\n",
           (unsigned long long)TRACEHUB_DEFAULT_RTT_SIZE);
    printf("  RTT backend path:          %s\n", TRACEHUB_DEFAULT_MEMORY_PATH);
    printf("  Linux text channel:        %u\n", RTT_BRIDGE_DEFAULT_LINUX_CHANNEL);
    printf("  RTOS text channel:         %u\n", RTT_BRIDGE_DEFAULT_RTOS_CHANNEL);
    printf("  Terminal text channel:     %u\n", RTT_BRIDGE_DEFAULT_TERMINAL_CHANNEL);
    printf("  SystemView channel:        %u\n", RTT_BRIDGE_DEFAULT_SYSVIEW_CHANNEL);
    printf("  Terminal Telnet port:      %u\n", RTT_BRIDGE_DEFAULT_TERMINAL_PORT);
    printf("  SystemView TCP port:       %u\n", RTT_BRIDGE_DEFAULT_SYSVIEW_PORT);
    printf("  Swimlane non-TTY width:    %u\n", (unsigned)SWIMLANE_DEFAULT_TOTAL_WIDTH);
    printf("  Swimlane timestamp width:  %u\n", (unsigned)SWIMLANE_DEFAULT_TIMESTAMP_WIDTH);
    printf("  Linux swimlane label:      %s\n", SWIMLANE_DEFAULT_LINUX_LABEL);
    printf("  RTOS swimlane label:       %s\n", SWIMLANE_DEFAULT_RTOS_LABEL);
    printf("\n");
    printf("Examples:\n");
    printf("  # Default dual-source swimlane using build-time RTT defaults\n");
    printf("  tracehub\n");
    printf("\n");
    printf("  # Console mode, RTOS logs, host MEMSHM simulation\n");
    printf("  tracehub --shm /rtt_sim --addr 0x10000000 --size 0 --rtos\n");
    printf("\n");
    printf("  # Dual-source swimlane with local SystemView recording and TCP streaming\n");
    printf("  tracehub --shm /rtt_sim --addr 0x10000000 --size 0 --swimlane --systemview-port 19111\n");
    printf("\n");
    printf("  # Linux SMEM real-chip path through the SharedMem driver\n");
    printf("  tracehub --shm /dev/shared_mem0 --addr 0x10040000 --size 0x20000 --swimlane\n");
#if defined(RTTMEM_USE_MEMSHM)
    printf("\n");
    printf("  # Reset the MEMSHM simulation object before attaching\n");
    printf("  tracehub --shm /rtt_sim --addr 0x10000000 --size 0 --memshm-reset --swimlane\n");
#endif
    printf("\n");
    printf("Note:\n");
    printf("  - TraceHub runs on Linux, macOS, or Windows hosts.\n");
    printf("  - SMEM accesses real-chip RTT memory through a Linux SharedMem driver.\n");
    printf("  - macOS and Windows builds support MEMSHM host simulation only.\n");
    printf("  - --console, --swimlane, and --telnet-port are mutually exclusive modes.\n");
    printf("  - --linux and --rtos are single-source selectors and cannot be used with --swimlane.\n");
    printf("  - In single-source modes, Linux and RTOS source options are mutually exclusive.\n");
    printf("  - The SMEM backend may require root privileges to access mapped memory.\n");
    printf("  - Real-chip swimlane logs require both text sources to use the same hardware time base.\n");
    printf("  - Text log files strip terminal control sequences; live Terminal output preserves target colors.\n");
    printf("  - Queue sizes, swimlane timestamp width, flush thresholds, and source labels are build-time defaults.\n");
    printf("\n");
    printf("Support: wenshuaisong@gmail.com\n");
    printf("\n");
}

/*************************** End of file ****************************/
