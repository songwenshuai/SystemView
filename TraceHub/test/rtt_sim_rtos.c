/*********************************************************************
*                                                                    *
*                Copyright (C) 2023 xrTest Inc.                      *
*                      All rights reserved                           *
*                                                                    *
**********************************************************************
----------------------------------------------------------------------
File    : rtt_sim_rtos.c
Purpose : Simulate RTOS R5 core writing logs to RTT channel 1
          and SystemView multi-core events to RTT channel 2.
          RTOS only VALIDATES RTT CB - never initializes.
          Waits for Linux app to initialize RTTCB first.
          Fails fast with clear error if RTTCB not available.
Author  : songwenshuai <songwenshuai@gmail.com>

Usage   :
          # Startup order: tracehub -> Linux app -> RTOS
          # Linux app initializes RTTCB, RTOS only validates
          ./tracehub --addr 0x10000000 --size 0 --shm /rtt_sim --swimlane &
          ./rtt_sim_linux   # Sole initializer - must start first
          ./rtt_sim_rtos    # Validate only, never initialize
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
#include "rtt_sim_common.h"
#include "rtt_sim_sysview.h"

/*********************************************************************
*
*       Defines
*
**********************************************************************
*/

#define SIM_SHM_NAME            "/rtt_sim"
#define SIM_BASE_ADDRESS        UINT64_C(0x10000000)
#define SIM_RTOS_CHANNEL        1
#define SIM_LINUX_CHANNEL       0
#define SIM_SYSVIEW_CHANNEL     2
#define SIM_LOG_INTERVAL_MS     20
#define SIM_NUM_CHANNELS        3
#define SIM_SYSVIEW_CPU_FREQ_HZ UINT32_C(400000000)
#define SIM_SYSVIEW_RAM_BASE    UINT32_C(0x00000000)
#define SIM_SYSVIEW_TASK_BASE   UINT32_C(0x20010000)
#define SIM_SYSVIEW_STACK_BASE  UINT32_C(0x20080000)
#define SIM_SYSVIEW_MARKER_BASE UINT32_C(0x20001000)
#define SIM_SYSVIEW_TIMER_BASE  UINT32_C(0x20020000)
#define SIM_SYSVIEW_IRQ_BASE    64u

//
// RTTCB wait configuration
// RTOS waits for Linux app to initialize RTTCB
//
#define SIM_RTTCB_WAIT_INTERVAL_MS   1000    // Wait interval between retries
#define SIM_RTTCB_MAX_WAIT_RETRIES   60      // Max retries (0 = infinite)

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
*/
static void _SignalHandler(int signum) {
    (void)signum;
    printf("\nRTOS Simulator: received signal, stopping...\n");
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
    unsigned sysview_cycle_count = 0;
    unsigned load_percent;
    unsigned sysview_bytes;
    uint64_t timestamp_us;
    char     log_msg[256];
    unsigned message_len;
    unsigned bytes_written;
    int      exit_code = 0;
    uintptr_t rtt_address;
    size_t    rtt_region_size;
    RTT_SIM_SYSVIEW_CoreConfig_t sysview_config;

    (void)argc;
    (void)argv;

    printf("==============================================\n");
    printf("  RTT RTOS R5 Simulator (VALIDATE ONLY)\n");
    printf("==============================================\n");
    printf("  Role:          Consumer (never initializes RTTCB)\n");
    printf("  Shared Memory: %s\n", SIM_SHM_NAME);
    printf("  Backend Addr:  0x%016" PRIx64 "\n", SIM_BASE_ADDRESS);
    printf("  RTT Channel:   %d (RTOS)\n", SIM_RTOS_CHANNEL);
    printf("  RTT Channel:   %d (SystemView trace)\n", SIM_SYSVIEW_CHANNEL);
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
    // Wait for RTT CB to be initialized by Linux app
    // RTOS never initializes - only validates (consumer mode)
    //
    printf("\n[RTOS] Waiting for RTT CB and channel %d (consumer mode)...\n", SIM_RTOS_CHANNEL);
    printf("[RTOS] NOTE: RTOS never initializes - Linux app is sole owner\n");
    ret = RTTBridge_WaitForRTTUpChannelReady(rtt_address,
                                             rtt_region_size,
                                             SIM_RTOS_CHANNEL,
                                             SIM_RTTCB_MAX_WAIT_RETRIES * SIM_RTTCB_WAIT_INTERVAL_MS,
                                             SIM_RTTCB_WAIT_INTERVAL_MS);
    if (ret != 0) {
        fprintf(stderr, "[RTOS] ERROR: RTT CB or channel %d not ready\n", SIM_RTOS_CHANNEL);
        fprintf(stderr, "[RTOS] Please ensure startup order:\n");
        fprintf(stderr, "[RTOS]   1. Start tracehub (waits for RTTCB)\n");
        fprintf(stderr, "[RTOS]   2. Start rtt_sim_linux (sole initializer)\n");
        fprintf(stderr, "[RTOS]   3. Start rtt_sim_rtos (this program)\n");
        RTTMem_Cleanup();
        return 1;
    }
    printf("[RTOS] Found valid RTT CB and channel %d\n", SIM_RTOS_CHANNEL);
    printf("[RTOS] Channel %d (Linux) available\n", SIM_LINUX_CHANNEL);
    printf("[RTOS] Channel %d (RTOS) available\n", SIM_RTOS_CHANNEL);
    ret = RTTBridge_WaitForRTTUpChannelReady(rtt_address,
                                             rtt_region_size,
                                             SIM_SYSVIEW_CHANNEL,
                                             SIM_RTTCB_MAX_WAIT_RETRIES * SIM_RTTCB_WAIT_INTERVAL_MS,
                                             SIM_RTTCB_WAIT_INTERVAL_MS);
    if (ret != 0) {
        fprintf(stderr, "[RTOS] ERROR: RTT CB or SystemView channel %d not ready\n", SIM_SYSVIEW_CHANNEL);
        RTTMem_Cleanup();
        return 1;
    }
    printf("[RTOS] Channel %d (SystemView trace) available\n", SIM_SYSVIEW_CHANNEL);

    memset(&sysview_config, 0, sizeof(sysview_config));
    sysview_config.rtt_address      = rtt_address;
    sysview_config.rtt_region_size  = rtt_region_size;
    sysview_config.channel          = SIM_SYSVIEW_CHANNEL;
    sysview_config.num_channels     = SIM_NUM_CHANNELS;
    sysview_config.core_id          = 1u;
    sysview_config.lock_owner       = false;
    sysview_config.core_name        = "R5";
    sysview_config.application_name = "tracehub-multicore-sim";
    sysview_config.device_name      = "MEMSHM heterogeneous target";
    sysview_config.os_name          = "RTOS";
    sysview_config.cpu_freq_hz      = SIM_SYSVIEW_CPU_FREQ_HZ;
    sysview_config.ram_base         = SIM_SYSVIEW_RAM_BASE;
    sysview_config.task_base        = SIM_SYSVIEW_TASK_BASE;
    sysview_config.stack_base       = SIM_SYSVIEW_STACK_BASE;
    sysview_config.marker_base      = SIM_SYSVIEW_MARKER_BASE;
    sysview_config.timer_base       = SIM_SYSVIEW_TIMER_BASE;
    sysview_config.interrupt_base   = SIM_SYSVIEW_IRQ_BASE;
    if (RTT_SIM_SYSVIEW_StartCore(&sysview_config) != 0) {
        fprintf(stderr, "[RTOS] ERROR: Failed to start SystemView event generator\n");
        RTTMem_Cleanup();
        return 1;
    }
    snprintf(log_msg, sizeof(log_msg),
             "RTOS R5 boot banner without timestamp\r\n");
    message_len = (unsigned)strlen(log_msg);
    bytes_written = SEGGER_RTT_WriteNoLock(rtt_address, SIM_RTOS_CHANNEL,
                                           log_msg, message_len);
    if (bytes_written != message_len) {
        fprintf(stderr,
                "[RTOS] ERROR: RTT channel %d accepted %u/%u startup log bytes; stopping to avoid log loss\n",
                SIM_RTOS_CHANNEL,
                bytes_written,
                message_len);
        RTT_SIM_SYSVIEW_StopCore();
        RTTMem_Cleanup();
        return 1;
    }
    //
    // Main loop - write RTOS logs to channel 1
    //
    printf("\nStarting RTOS log output (Press Ctrl+C to stop)...\n\n");

    while (_running) {
        next_log_count = log_count + 1u;
        timestamp_us = SYS_GetMonotonicTimeUs();
        load_percent = (next_log_count * 11u) % 100u;
        (void)RTT_SIM_DrainDownBuffer(rtt_address, SIM_RTOS_CHANNEL, "RTOS");
        //
        // Format RTOS log message
        //
        snprintf(log_msg, sizeof(log_msg),
                 "[%" PRIu64 "] \033[33mRTOS R5\033[0m: Task scheduler tick #%u, heap free: %u bytes\r\n",
                 timestamp_us, next_log_count, 8192 - (next_log_count % 1024));
        message_len = (unsigned)strlen(log_msg);
        //
        // Write to RTT channel 1 (RTOS)
        //
        bytes_written = SEGGER_RTT_WriteNoLock(rtt_address, SIM_RTOS_CHANNEL,
                                               log_msg, message_len);

        if (bytes_written == message_len) {
            printf("RTOS [ch%d]: %s", SIM_RTOS_CHANNEL, log_msg);
            log_count = next_log_count;
        } else {
            fprintf(stderr,
                    "[RTOS] ERROR: RTT channel %d accepted %u/%u log bytes; stopping to avoid log loss\n",
                    SIM_RTOS_CHANNEL,
                    bytes_written,
                    message_len);
            exit_code = 1;
            break;
        }

        sysview_cycle_count++;
        sysview_bytes = RTT_SIM_SYSVIEW_RecordCycle(sysview_cycle_count, load_percent);
        printf("SystemView [ch%d R5]: event cycle #%u (%u bytes)\n",
               SIM_SYSVIEW_CHANNEL, sysview_cycle_count, sysview_bytes);

        SYS_Sleep(SIM_LOG_INTERVAL_MS);
    }
    //
    // Cleanup
    //
    RTT_SIM_SYSVIEW_StopCore();
    printf("\nRTOS Simulator shutting down...\n");
    printf("Total logs sent: %u\n", log_count);
    printf("Total SystemView cycles sent: %u\n", sysview_cycle_count);
    printf("Total SystemView bytes recorded: %" PRIu64 "\n", RTT_SIM_SYSVIEW_GetRecordedBytes());
    RTTMem_Cleanup();
    printf("Cleanup complete\n");

    return exit_code;
}

/*************************** End of file ****************************/
