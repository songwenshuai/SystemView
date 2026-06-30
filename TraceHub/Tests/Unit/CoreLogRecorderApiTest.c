/*********************************************************************
*                             CineLogic                              *
*                      RTT Trace and Debug Bridge                    *
**********************************************************************
----------------------------------------------------------------------
File    : CoreLogRecorderApiTest.c
Purpose : Unit checks for core log recorder public APIs
---------------------------END-OF-HEADER------------------------------
*/

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "CoreLogRecorder_internal.h"
#include "RTTBridge.h"
#include "TestCommon.h"

static int               _stub_check_channel_result;
static RTTBridge_State_t _stub_bridge_state;

int RTTBridge_CheckUpBufferChannel(unsigned channel) {
    (void)channel;
    return _stub_check_channel_result;
}

int RTTBridge_ReadUpBufferNoLock(unsigned channel, void *buffer, size_t buffer_size) {
    (void)channel;
    (void)buffer;
    (void)buffer_size;
    return -1;
}

const RTTBridge_State_t *RTTBridge_GetState(void) {
    return &_stub_bridge_state;
}

bool RTTBridge_IsRunning(void) {
    return false;
}

static void _FillConfig(CoreLogRecorder_Config_t *config) {
    memset(config, 0, sizeof(*config));
    config->linux_channel = 3u;
    config->rtos_channel = 4u;
    config->poll_interval_ms = 1u;
    config->linux_prefix = "tracehub_unit_core_linux";
    config->rtos_prefix = "tracehub_unit_core_rtos";
    config->linux_enabled = true;
    config->rtos_enabled = false;
}

static int _TestRejectInvalidConfiguration(void) {
    CoreLogRecorder_Config_t config;

    CoreLogRecorder_Cleanup();
    TEST_ASSERT(CoreLogRecorder_Init(NULL) == -1);

    memset(&config, 0, sizeof(config));
    TEST_ASSERT(CoreLogRecorder_Init(&config) == -1);

    _FillConfig(&config);
    config.rtos_enabled = true;
    config.rtos_channel = config.linux_channel;
    TEST_ASSERT(CoreLogRecorder_Init(&config) == -1);
    TEST_ASSERT(CoreLogRecorder_GetStats(NULL) == -1);
    TEST_ASSERT(!CoreLogRecorder_IsRunning());
    TEST_ASSERT(!CoreLogRecorder_HasFatalError());
    return 0;
}

static int _TestConsumerQueueStatsAndCleanup(void) {
    CoreLogRecorder_Config_t config;
    CoreLogRecorder_Stats_t  stats;
    char                     buffer[16];
    int                      result;

    CoreLogRecorder_Cleanup();
    _FillConfig(&config);
    TEST_ASSERT(CoreLogRecorder_Init(&config) == 0);
    TEST_ASSERT(CoreLogRecorder_IsCoreChannel(3u));
    TEST_ASSERT(!CoreLogRecorder_IsCoreChannel(4u));
    TEST_ASSERT(CoreLogRecorder_RegisterConsumer(4u) == -1);
    TEST_ASSERT(CoreLogRecorder_RegisterConsumer(3u) == 0);
    TEST_ASSERT(CoreLogRecorder_RegisterConsumer(3u) == -2);
    TEST_ASSERT(CoreLogRecorder_ReadChannel(3u, buffer, sizeof(buffer)) == 0);

    _Recorder_RecordBytes(CORE_LOG_RECORDER_LINUX, "abc", 3u);
    result = CoreLogRecorder_ReadChannel(3u, buffer, sizeof(buffer));
    TEST_ASSERT(result == 3);
    TEST_ASSERT(memcmp(buffer, "abc", 3u) == 0);
    TEST_ASSERT(CoreLogRecorder_ReadChannel(3u, buffer, sizeof(buffer)) == 0);

    TEST_ASSERT(CoreLogRecorder_GetStats(&stats) == 0);
    TEST_ASSERT(stats.linux.enabled);
    TEST_ASSERT(stats.linux.channel == 3u);
    TEST_ASSERT(stats.linux.bytes_recorded == 3u);
    TEST_ASSERT(stats.linux.bytes_consumed == 3u);
    TEST_ASSERT(stats.linux.consumer_registered);
    TEST_ASSERT(!stats.rtos.enabled);
    TEST_ASSERT(!stats.fatal_error);

    CoreLogRecorder_UnregisterConsumer(3u);
    TEST_ASSERT(CoreLogRecorder_ReadChannel(3u, buffer, sizeof(buffer)) == -2);
    TEST_ASSERT(CoreLogRecorder_GetStats(&stats) == 0);
    TEST_ASSERT(!stats.linux.consumer_registered);

    CoreLogRecorder_Cleanup();
    TEST_ASSERT(CoreLogRecorder_GetStats(&stats) == -1);
    TEST_ASSERT(!CoreLogRecorder_IsCoreChannel(3u));
    return 0;
}

static int _TestStartStopContracts(void) {
    CoreLogRecorder_Config_t config;

    CoreLogRecorder_Cleanup();
    TEST_ASSERT(CoreLogRecorder_Start() == -1);
    CoreLogRecorder_Stop();

    _FillConfig(&config);
    TEST_ASSERT(CoreLogRecorder_Init(&config) == 0);
    _stub_check_channel_result = -1;
    TEST_ASSERT(CoreLogRecorder_Start() == -3);
    TEST_ASSERT(CoreLogRecorder_HasFatalError());
    TEST_ASSERT(!CoreLogRecorder_IsRunning());
    CoreLogRecorder_Stop();
    CoreLogRecorder_Cleanup();

    _stub_check_channel_result = 0;
    _FillConfig(&config);
    TEST_ASSERT(CoreLogRecorder_Init(&config) == 0);
    TEST_ASSERT(CoreLogRecorder_Start() == 0);
    TEST_ASSERT(CoreLogRecorder_IsRunning());
    CoreLogRecorder_Stop();
    TEST_ASSERT(!CoreLogRecorder_IsRunning());
    CoreLogRecorder_Cleanup();
    return 0;
}

int main(void) {
    memset(&_stub_bridge_state, 0, sizeof(_stub_bridge_state));
    _stub_bridge_state.config.rtt_search_timeout_ms = 1u;
    TEST_RUN(_TestRejectInvalidConfiguration);
    TEST_RUN(_TestConsumerQueueStatsAndCleanup);
    TEST_RUN(_TestStartStopContracts);
    return 0;
}

/*************************** End of file ****************************/
