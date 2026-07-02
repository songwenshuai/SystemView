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
File    : RTTSimLinux.c
Purpose : Simulate Linux logs written to RTT channel 0
          Linux app is the SOLE INITIALIZER of RTT control block.
          First startup: initializes RTTCB.
          Restart: reuses existing valid RTTCB (no re-init).
---------------------------END-OF-HEADER------------------------------
*/

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "RTTBridge.h"
#include "RTTMemory.h"
#include "SYS.h"
#include "SEGGER_RTT.h"
#include "RTTSimCommon.h"

/*********************************************************************
*
*       Defines
*
**********************************************************************
*/

#define SIM_SHM_NAME            "/rtt_sim"
#define SIM_BASE_ADDRESS        UINT64_C(0x10000000)
#define SIM_LINUX_CHANNEL       0
#define SIM_RTOS_CHANNEL        1
#define SIM_LOG_INTERVAL_MS     50
#define SIM_NUM_CHANNELS        2

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
*       _SignalHandler()
*
*  Function description
*    Request simulator shutdown when a termination signal is received.
*
*  Parameters
*    signum  Signal number.
*/
static void _SignalHandler(int signum) {
    (void)signum;
    printf("\nLinux Simulator: received signal, stopping...\n");
    _running = 0;
}


/*********************************************************************
*
*       Public code
*
**********************************************************************
*/

/*********************************************************************
*
*       main
*/
int main(int argc, char *argv[]) {
    int      ret;
    unsigned log_count = 0;
    unsigned next_log_count;
    unsigned load_percent;
    uint64_t timestamp_us;
    char     log_msg[256];
    unsigned message_len;
    unsigned bytes_written;
    int      exit_code = 0;
    uintptr_t rtt_address;
    size_t    rtt_region_size;

    (void)argc;
    (void)argv;

    printf("==============================================\n");
    printf("  RTT Linux Simulator (SOLE INITIALIZER)\n");
    printf("==============================================\n");
    printf("  Role:          RTTCB Owner (init or reuse)\n");
    printf("  Shared Memory: %s\n", SIM_SHM_NAME);
    printf("  Backend Addr:  0x%016" PRIx64 "\n", SIM_BASE_ADDRESS);
    printf("  RTT Channel:   %d (Linux)\n", SIM_LINUX_CHANNEL);
    printf("==============================================\n\n");
    //
    // Setup signal handler (don't restart syscalls after signal)
    //
    struct sigaction sa;
    sa.sa_handler = _SignalHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;  // No SA_RESTART - allow usleep to be interrupted
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    //
    // Install MEMSHM backend
    //
    ret = RTTMem_InstallBackend(RTTMem_Backend_MEMSHM());
    if (ret != 0) {
        fprintf(stderr, "Failed to install MEMSHM backend\n");
        return 1;
    }
    printf("Memory backend: %s\n", RTTMem_GetBackendName());
    //
    // Initialize shared memory
    //
    ret = RTTMem_InitEx(SIM_SHM_NAME, SIM_BASE_ADDRESS, 0u);
    if (ret != 0) {
        fprintf(stderr, "Failed to initialize shared memory\n");
        return 1;
    }

    printf("Shared memory initialized\n");
    printf("  Backend base:  0x%016" PRIx64 "\n", RTTMem_GetBackendBase());
    printf("  Mapped size:   %zu bytes\n", RTTMem_GetMappedSize());

    rtt_region_size = RTTMem_GetMappedSize();
    rtt_address = RTTMem_ToLocalAddress(RTTMem_GetBackendBase(), rtt_region_size);
    if (rtt_address == 0) {
        fprintf(stderr, "Failed to convert RTT backend address to local mapping\n");
        RTTMem_Cleanup();
        return 1;
    }
    printf("  Local address: 0x%" PRIxPTR "\n", rtt_address);
    //
    // Ensure RTT CB is ready (init if first startup, reuse if restart)
    // Linux is the SOLE OWNER of RTTCB initialization
    //
    printf("\n[Linux] Ensuring RTT CB is ready (owner mode)...\n");
    ret = RTTBridge_EnsureRTTInitialized(rtt_address, rtt_region_size, SIM_NUM_CHANNELS);
    if (ret != 0) {
        fprintf(stderr, "[Linux] ERROR: Failed to ensure RTT CB\n");
        RTTMem_Cleanup();
        return 1;
    }
    printf("[Linux] RTT CB ready\n");
    printf("[Linux] Channel %d (Linux) available\n", SIM_LINUX_CHANNEL);
    printf("[Linux] Channel %d (RTOS) available\n", SIM_RTOS_CHANNEL);
    snprintf(log_msg, sizeof(log_msg),
             "Linux boot banner without timestamp\r\n");
    message_len = (unsigned)strlen(log_msg);
    bytes_written = SEGGER_RTT_WriteNoLock(rtt_address, SIM_LINUX_CHANNEL,
                                           log_msg, message_len);
    if (bytes_written != message_len) {
        fprintf(stderr,
                "[Linux] ERROR: RTT channel %d accepted %u/%u startup log bytes; stopping to avoid log loss\n",
                SIM_LINUX_CHANNEL,
                bytes_written,
                message_len);
        RTTMem_Cleanup();
        return 1;
    }
    //
    // Main loop - write Linux logs to channel 0
    //
    printf("\nStarting Linux log output (Press Ctrl+C to stop)...\n\n");

    while (_running) {
        next_log_count = log_count + 1u;
        timestamp_us = SYS_GetMonotonicTimeUs();
        load_percent = (next_log_count * 7u) % 100u;
        (void)RTT_SIM_DrainDownBuffer(rtt_address, SIM_LINUX_CHANNEL, "Linux");
        //
        // Format Linux log message
        //
        snprintf(log_msg, sizeof(log_msg),
                 "[%" PRIu64 "] \033[32mLinux\033[0m: Application event #%u, CPU load: %u%%\r\n",
                 timestamp_us, next_log_count, load_percent);
        message_len = (unsigned)strlen(log_msg);
        //
        // Write to RTT channel 0 (Linux)
        //
        bytes_written = SEGGER_RTT_WriteNoLock(rtt_address, SIM_LINUX_CHANNEL,
                                               log_msg, message_len);

        if (bytes_written == message_len) {
            printf("Linux [ch%d]: %s", SIM_LINUX_CHANNEL, log_msg);
            log_count = next_log_count;
        } else {
            fprintf(stderr,
                    "[Linux] ERROR: RTT channel %d accepted %u/%u log bytes; stopping to avoid log loss\n",
                    SIM_LINUX_CHANNEL,
                    bytes_written,
                    message_len);
            exit_code = 1;
            break;
        }

        SYS_Sleep(SIM_LOG_INTERVAL_MS);
    }
    //
    // Cleanup
    //
    printf("\nLinux Simulator shutting down...\n");
    printf("Total logs sent: %u\n", log_count);
    RTTMem_Cleanup();
    printf("Cleanup complete\n");

    return exit_code;
}

/*************************** End of file ****************************/
