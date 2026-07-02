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

static int _TestInternalQueueWrapAndCapacityFailure(void) {
    CoreLogRecorder_Config_t  config;
    CoreLogRecorder_Stats_t   stats;
    CoreLogRecorder_Source_t *source;
    char                      buffer[8];

    CoreLogRecorder_Cleanup();
    _FillConfig(&config);
    TEST_ASSERT(CoreLogRecorder_Init(&config) == 0);
    TEST_ASSERT(CoreLogRecorder_RegisterConsumer(3u) == 0);

    source = &_recorder_state.sources[CORE_LOG_RECORDER_LINUX];
    TEST_ASSERT(_Recorder_QueueBytesLocked(NULL, "x", 1u) == 0);
    TEST_ASSERT(_Recorder_QueueBytesLocked(source, NULL, 1u) == 0);
    TEST_ASSERT(_Recorder_QueueBytesLocked(source, "x", 0u) == 0);

    SYS_MutexLock(&source->lock);
    source->queue_read = source->queue_size - 2u;
    source->queue_used = 0u;
    TEST_ASSERT(_Recorder_QueueBytesLocked(source, "abcd", 4u) == 0);
    SYS_MutexUnlock(&source->lock);

    memset(buffer, 0, sizeof(buffer));
    TEST_ASSERT(CoreLogRecorder_ReadChannel(3u, buffer, 2u) == 2);
    TEST_ASSERT(memcmp(buffer, "ab", 2u) == 0);
    TEST_ASSERT(CoreLogRecorder_ReadChannel(3u, buffer, sizeof(buffer)) == 2);
    TEST_ASSERT(memcmp(buffer, "cd", 2u) == 0);

    SYS_MutexLock(&source->lock);
    source->queue_read = 0u;
    source->queue_used = source->queue_size - 1u;
    TEST_ASSERT(_Recorder_QueueBytesLocked(source, "xy", 2u) == -1);
    SYS_MutexUnlock(&source->lock);

    TEST_ASSERT(CoreLogRecorder_HasFatalError());
    TEST_ASSERT(CoreLogRecorder_GetStats(&stats) == 0);
    TEST_ASSERT(stats.linux.consumer_undelivered_bytes == 2u);
    TEST_ASSERT(stats.linux.consumer_capacity_failure_count == 1u);
    TEST_ASSERT(CoreLogRecorder_ReadChannel(3u, buffer, sizeof(buffer)) == -3);

    SYS_MutexLock(&source->lock);
    _Recorder_ClearConsumerQueueLocked(source);
    _Recorder_ReportFileErrorLocked(source, NULL);
    TEST_ASSERT(source->file_error);
    TEST_ASSERT(source->file_error_logged);
    SYS_MutexUnlock(&source->lock);

    _Recorder_RecordBytes(CORE_LOG_RECORDER_RTOS, "ignored", 7u);
    _Recorder_RecordBytes(CORE_LOG_RECORDER_NUM_SOURCES, "ignored", 7u);
    _Recorder_RecordBytes(CORE_LOG_RECORDER_LINUX, NULL, 7u);
    _Recorder_RecordBytes(CORE_LOG_RECORDER_LINUX, "ignored", 0u);

    CoreLogRecorder_UnregisterConsumer(3u);
    CoreLogRecorder_Cleanup();
    return 0;
}

static int _TestStateAndRegistryInternalContracts(void) {
    CoreLogRecorder_Source_t *source;

    CoreLogRecorder_Cleanup();
    memset(&_recorder_state, 0, sizeof(_recorder_state));

    _recorder_state.running = true;
    TEST_ASSERT(_Recorder_IsRunning());
    _Recorder_SetRunning(false);
    TEST_ASSERT(!_Recorder_IsRunning());
    TEST_ASSERT(!_Recorder_HasFatalError());
    _Recorder_SetFatalError();
    TEST_ASSERT(_Recorder_HasFatalError());
    TEST_ASSERT(!_Recorder_IsRunning());

    memset(&_recorder_state, 0, sizeof(_recorder_state));
    TEST_ASSERT(_Recorder_GetPollInterval() == RTT_BRIDGE_DEFAULT_POLL_INTERVAL_MS);
    _recorder_state.config.poll_interval_ms = 77u;
    TEST_ASSERT(_Recorder_GetPollInterval() == 77u);

    memset(&_recorder_state, 0, sizeof(_recorder_state));
    _recorder_state.sources[CORE_LOG_RECORDER_LINUX].enabled = true;
    _recorder_state.sources[CORE_LOG_RECORDER_LINUX].channel = 3u;
    _recorder_state.sources[CORE_LOG_RECORDER_RTOS].enabled = true;
    _recorder_state.sources[CORE_LOG_RECORDER_RTOS].channel = 4u;
    TEST_ASSERT(_Recorder_FindSource(3u) == &_recorder_state.sources[CORE_LOG_RECORDER_LINUX]);
    TEST_ASSERT(_Recorder_FindSource(4u) == &_recorder_state.sources[CORE_LOG_RECORDER_RTOS]);
    TEST_ASSERT(_Recorder_FindSource(5u) == NULL);
    TEST_ASSERT(_Recorder_SourceIsEnabled(CORE_LOG_RECORDER_LINUX));
    TEST_ASSERT(_Recorder_SourceIsEnabled(CORE_LOG_RECORDER_RTOS));
    TEST_ASSERT(!_Recorder_SourceIsEnabled(CORE_LOG_RECORDER_NUM_SOURCES));

    TEST_ASSERT(SYS_MutexInit(&_recorder_state.sources[CORE_LOG_RECORDER_LINUX].lock) == 0);
    _recorder_state.sources[CORE_LOG_RECORDER_LINUX].lock_initialized = true;
    TEST_ASSERT(SYS_MutexInit(&_recorder_state.sources[CORE_LOG_RECORDER_RTOS].lock) == 0);
    _recorder_state.sources[CORE_LOG_RECORDER_RTOS].lock_initialized = true;
    _Recorder_DestroySourceLocks();
    TEST_ASSERT(!_recorder_state.sources[CORE_LOG_RECORDER_LINUX].lock_initialized);
    TEST_ASSERT(!_recorder_state.sources[CORE_LOG_RECORDER_RTOS].lock_initialized);

    memset(&_recorder_state, 0, sizeof(_recorder_state));
    TEST_ASSERT(_Recorder_InitSource(CORE_LOG_RECORDER_NUM_SOURCES,
                                    7u,
                                    false,
                                    "Invalid",
                                    NULL) == -1);
    TEST_ASSERT(_Recorder_InitSource(CORE_LOG_RECORDER_LINUX,
                                    8u,
                                    false,
                                    "Linux",
                                    NULL) == 0);
    source = &_recorder_state.sources[CORE_LOG_RECORDER_LINUX];
    TEST_ASSERT(source->channel == 8u);
    TEST_ASSERT(source->name != NULL);
    TEST_ASSERT(!source->enabled);
    TEST_ASSERT(!source->lock_initialized);
    TEST_ASSERT(_Recorder_InitSource(CORE_LOG_RECORDER_RTOS,
                                    9u,
                                    true,
                                    "RTOS",
                                    NULL) == -1);

    memset(&_recorder_state, 0, sizeof(_recorder_state));
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
    TEST_ASSERT(CoreLogRecorder_Start() == -1);
    CoreLogRecorder_Stop();
    TEST_ASSERT(!CoreLogRecorder_IsRunning());
    CoreLogRecorder_Cleanup();

    _stub_check_channel_result = 0;
    _FillConfig(&config);
    config.rtos_enabled = true;
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
    TEST_RUN(_TestInternalQueueWrapAndCapacityFailure);
    TEST_RUN(_TestStateAndRegistryInternalContracts);
    TEST_RUN(_TestStartStopContracts);
    return 0;
}

/*************************** End of file ****************************/
