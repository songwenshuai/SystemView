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
File    : LogCollectorTest.c
Purpose : Unit checks for log collector timestamp assignment
---------------------------END-OF-HEADER------------------------------
*/

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "CoreLogRecorder.h"
#include "RTTBridge.h"
#include "SYS.h"
#include "LogCollector_internal.h"

/*********************************************************************
*
*       Defines
*
**********************************************************************
*/

#define TEST_ASSERT(expr)                                                   \
    do {                                                                    \
        if (!(expr)) {                                                       \
            fprintf(stderr, "%s:%d: assertion failed: %s\n",                \
                    __FILE__, __LINE__, #expr);                             \
            return -1;                                                      \
        }                                                                   \
    } while (0)

#define TEST_MAX_ENTRIES         8u
#define TEST_CORE_CHANNEL_COUNT  4u

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

static bool        _test_core_registered[TEST_CORE_CHANNEL_COUNT];
static const char *_test_core_read_data[TEST_CORE_CHANNEL_COUNT];
static unsigned    _test_core_register_count;
static unsigned    _test_core_unregister_count;
static int         _test_thread_create_result[LOG_SOURCE_MAX];
static unsigned    _test_thread_create_count;
static unsigned    _test_thread_wait_count;
static bool        _test_thread_run_entry_on_create;
static unsigned    _test_rtt_running_calls_remaining;
static unsigned    _test_sleep_count;

/*********************************************************************
*
*       Static functions
*
**********************************************************************
*/

static void _ResetCoreRecorderStub(void) {
    memset(_test_core_registered, 0, sizeof(_test_core_registered));
    memset(_test_core_read_data, 0, sizeof(_test_core_read_data));
    _test_core_register_count = 0u;
    _test_core_unregister_count = 0u;
}

static void _SetCoreRecorderReadData(unsigned channel, const char *data) {
    if (channel < TEST_CORE_CHANNEL_COUNT) {
        _test_core_read_data[channel] = data;
    }
}

static void _ResetRuntimeStub(void) {
    unsigned i;

    for (i = 0u; i < LOG_SOURCE_MAX; i++) {
        _test_thread_create_result[i] = -1;
    }
    _test_thread_create_count = 0u;
    _test_thread_wait_count = 0u;
    _test_thread_run_entry_on_create = false;
    _test_rtt_running_calls_remaining = 0u;
    _test_sleep_count = 0u;
}

/*********************************************************************
*
*       Runtime dependency stubs
*
**********************************************************************
*/

int CoreLogRecorder_RegisterConsumer(unsigned channel) {
    if (channel >= TEST_CORE_CHANNEL_COUNT) {
        return -1;
    }
    if (_test_core_registered[channel]) {
        return -2;
    }

    _test_core_registered[channel] = true;
    _test_core_register_count++;
    return 0;
}

void CoreLogRecorder_UnregisterConsumer(unsigned channel) {
    if (channel < TEST_CORE_CHANNEL_COUNT && _test_core_registered[channel]) {
        _test_core_registered[channel] = false;
        _test_core_unregister_count++;
    }
}

int CoreLogRecorder_ReadChannel(unsigned channel, void *buffer, size_t buffer_size) {
    const char *data;
    size_t      len;

    if (channel >= TEST_CORE_CHANNEL_COUNT ||
        !_test_core_registered[channel] ||
        buffer == NULL) {
        return -1;
    }

    data = _test_core_read_data[channel];
    if (data == NULL || buffer_size == 0u) {
        return 0;
    }

    len = strlen(data);
    if (len > buffer_size) {
        len = buffer_size;
    }
    memcpy(buffer, data, len);
    _test_core_read_data[channel] = NULL;
    return (int)len;
}

bool RTTBridge_IsRunning(void) {
    if (_test_rtt_running_calls_remaining == 0u) {
        return false;
    }
    _test_rtt_running_calls_remaining--;
    return true;
}

int SYS_createThread(void (*threadEntry)(void *), void *context, SYS_Thread *pRetTid) {
    unsigned index;
    int      result;

    if (pRetTid != NULL) {
        memset(pRetTid, 0, sizeof(*pRetTid));
    }

    index = _test_thread_create_count;
    _test_thread_create_count++;
    if (index >= LOG_SOURCE_MAX) {
        return -1;
    }
    result = _test_thread_create_result[index];
    if (result == 0 && _test_thread_run_entry_on_create && threadEntry != NULL) {
        threadEntry(context);
    }
    return result;
}

void SYS_destroyThread(SYS_Thread handle) {
    (void)handle;
}

void SYS_ExitThread(void *status) {
    (void)status;
}

void SYS_WaitThreadTerm(SYS_Thread pRetTid) {
    (void)pRetTid;
    _test_thread_wait_count++;
}

unsigned SYS_GetTickCount(void) {
    return 0u;
}

uint64_t SYS_GetMonotonicTimeUs(void) {
    return 0u;
}

void SYS_Sleep(unsigned ms) {
    (void)ms;
    _test_sleep_count++;
}

int SYS_MutexInit(SYS_Mutex *mutex) {
    (void)mutex;
    return 0;
}

int SYS_MutexLock(SYS_Mutex *mutex) {
    (void)mutex;
    return 0;
}

int SYS_MutexUnlock(SYS_Mutex *mutex) {
    (void)mutex;
    return 0;
}

int SYS_MutexDestroy(SYS_Mutex *mutex) {
    (void)mutex;
    return 0;
}

void SYS_GetTimestampStr(char *buf, size_t size) {
    if (buf != NULL && size > 0u) {
        buf[0] = '\0';
    }
}

/*********************************************************************
*
*       Types
*
**********************************************************************
*/

typedef struct {
    LogEntry_t *entries[TEST_MAX_ENTRIES];
    unsigned    count;
} TestCollectorOutput_t;

/*********************************************************************
*
*       Static functions
*
**********************************************************************
*/

static int _StoreEntry(LogEntry_t *entry, void *user_data) {
    TestCollectorOutput_t *output;

    output = (TestCollectorOutput_t *)user_data;
    if (entry == NULL || output == NULL || output->count >= TEST_MAX_ENTRIES) {
        LogEntry_Destroy(entry);
        return -1;
    }

    output->entries[output->count] = entry;
    output->count++;
    return 0;
}

static void _DestroyStoredEntries(TestCollectorOutput_t *output) {
    unsigned i;

    if (output == NULL) {
        return;
    }

    for (i = 0u; i < output->count; i++) {
        LogEntry_Destroy(output->entries[i]);
        output->entries[i] = NULL;
    }
    output->count = 0u;
}

static void _ResetCollectorState(void) {
    LogCollectorState_ResetStorage();
    _ResetCoreRecorderStub();
    _ResetRuntimeStub();
}

static void _FillCollectorConfig(LogCollector_Config_t *config) {
    memset(config, 0, sizeof(*config));
    config->linux_channel = 0u;
    config->rtos_channel = 1u;
    config->poll_interval_ms = 1u;
}

static int _TestDefaultPendingUntimedCapacity(void) {
    LogCollector_Config_t config;
    LogCollector_State_t *state;

    _ResetCollectorState();
    memset(&config, 0, sizeof(config));
    config.linux_channel = 0u;
    config.rtos_channel = 1u;
    config.poll_interval_ms = 1u;

    TEST_ASSERT(LogCollector_Init(&config) == 0);
    state = LogCollectorState_Get();
    TEST_ASSERT(state->sources[LOG_SOURCE_LINUX].pending_untimed != NULL);
    TEST_ASSERT(state->sources[LOG_SOURCE_RTOS].pending_untimed != NULL);
    TEST_ASSERT(state->pending_untimed_size == LOG_COLLECTOR_DEFAULT_PENDING_UNTIMED_SIZE);

    LogCollector_Cleanup();
    TEST_ASSERT(state->sources[LOG_SOURCE_LINUX].pending_untimed == NULL);
    TEST_ASSERT(state->sources[LOG_SOURCE_RTOS].pending_untimed == NULL);
    TEST_ASSERT(state->pending_untimed_size == 0u);
    return 0;
}

static int _TestLifecycleWrapperBoundaryContracts(void) {
    LogCollector_Config_t config;
    LogCollector_State_t *state;

    _ResetCollectorState();
    _FillCollectorConfig(&config);
    TEST_ASSERT(LogCollector_Init(NULL) == -1);

    state = LogCollectorState_Get();
    state->running = true;
    TEST_ASSERT(LogCollector_Init(&config) == -1);

    _ResetCollectorState();
    _FillCollectorConfig(&config);
    TEST_ASSERT(LogCollector_Init(&config) == 0);
    state = LogCollectorState_Get();
    TEST_ASSERT(state->initialized);
    TEST_ASSERT(LogCollector_Init(&config) == 0);
    state = LogCollectorState_Get();
    TEST_ASSERT(state->initialized);
    TEST_ASSERT(state->pending_untimed_size == LOG_COLLECTOR_DEFAULT_PENDING_UNTIMED_SIZE);
    LogCollector_Cleanup();

    _ResetCollectorState();
    _FillCollectorConfig(&config);
    TEST_ASSERT(LogCollector_Init(&config) == 0);
    state = LogCollectorState_Get();
    state->sources[LOG_SOURCE_LINUX].thread_started = true;
    TEST_ASSERT(!LogCollector_IsRunning());
    LogCollector_Cleanup();
    TEST_ASSERT(_test_thread_wait_count == 1u);
    TEST_ASSERT(!state->sources[LOG_SOURCE_LINUX].thread_started);
    TEST_ASSERT(!state->initialized);

    _ResetCollectorState();
    return 0;
}

static int _TestPendingUntimedCapacityKeepsTrailingSentinel(void) {
    LogCollector_SourceState_t source_state;
    char                       pending[8];
    char                       content[6];
    size_t                     pending_len;
    size_t                     valid_content_len;
    int                        result;

    _ResetCollectorState();
    memset(&source_state, 0, sizeof(source_state));
    memset(pending, 0, sizeof(pending));
    memset(content, 'A', sizeof(content));

    source_state.source = LOG_SOURCE_LINUX;
    source_state.pending_untimed = pending;
    source_state.pending_untimed_size = sizeof(pending);
    valid_content_len = sizeof(pending) - LOG_COLLECTOR_PENDING_RECORD_OVERHEAD;
    result = LogCollectorSource_AppendPendingUntimedLine(&source_state,
                                                         content,
                                                         valid_content_len,
                                                         false,
                                                         false);
    pending_len = source_state.pending_untimed_len;
    TEST_ASSERT(result == 0);
    TEST_ASSERT(pending_len == valid_content_len + LOG_COLLECTOR_PENDING_RECORD_OVERHEAD - 1u);
    TEST_ASSERT(pending[pending_len] == '\0');

    memset(pending, 0, sizeof(pending));
    source_state.pending_untimed_len = 0u;
    result = LogCollectorSource_AppendPendingUntimedLine(&source_state,
                                                         content,
                                                         valid_content_len + 1u,
                                                         false,
                                                         false);
    TEST_ASSERT(result == LOG_COLLECT_RESULT_PENDING_OVERFLOW);

    _ResetCollectorState();
    return 0;
}

static int _TestPollKeepsConsumersRegisteredBetweenCalls(void) {
    LogCollector_Config_t  config;
    TestCollectorOutput_t  output;
    int                    result;

    _ResetCollectorState();
    memset(&config, 0, sizeof(config));
    memset(&output, 0, sizeof(output));
    config.linux_channel = 0u;
    config.rtos_channel = 1u;
    config.poll_interval_ms = 1u;

    TEST_ASSERT(LogCollector_Init(&config) == 0);

    _SetCoreRecorderReadData(0u, "[10] linux line one\n");
    result = LogCollector_Poll(_StoreEntry, &output);
    TEST_ASSERT(result == 1);
    TEST_ASSERT(output.count == 1u);
    TEST_ASSERT(_test_core_register_count == 2u);
    TEST_ASSERT(_test_core_unregister_count == 0u);
    TEST_ASSERT(_test_core_registered[0u]);
    TEST_ASSERT(_test_core_registered[1u]);
    _DestroyStoredEntries(&output);

    _SetCoreRecorderReadData(0u, "[11] linux line two\n");
    result = LogCollector_Poll(_StoreEntry, &output);
    TEST_ASSERT(result == 1);
    TEST_ASSERT(output.count == 1u);
    TEST_ASSERT(_test_core_register_count == 2u);
    TEST_ASSERT(_test_core_unregister_count == 0u);
    TEST_ASSERT(_test_core_registered[0u]);
    TEST_ASSERT(_test_core_registered[1u]);
    _DestroyStoredEntries(&output);

    LogCollector_Cleanup();
    TEST_ASSERT(_test_core_unregister_count == 2u);
    TEST_ASSERT(!_test_core_registered[0u]);
    TEST_ASSERT(!_test_core_registered[1u]);
    return 0;
}

static int _TestStartStopThreadLifecycle(void) {
    LogCollector_Config_t config;
    LogCollector_State_t *state;
    TestCollectorOutput_t output;
    unsigned              i;

    _ResetCollectorState();
    memset(&output, 0, sizeof(output));
    _FillCollectorConfig(&config);
    TEST_ASSERT(LogCollector_Init(&config) == 0);

    TEST_ASSERT(LogCollector_Start(NULL, &output) == -1);
    for (i = 0u; i < LOG_SOURCE_MAX; i++) {
        _test_thread_create_result[i] = 0;
    }
    TEST_ASSERT(LogCollector_Start(_StoreEntry, &output) == 0);
    TEST_ASSERT(LogCollector_IsRunning());
    TEST_ASSERT(_test_thread_create_count == LOG_SOURCE_MAX);
    TEST_ASSERT(_test_core_register_count == 2u);

    state = LogCollectorState_Get();
    TEST_ASSERT(state->callback == _StoreEntry);
    TEST_ASSERT(state->user_data == &output);
    TEST_ASSERT(state->sources[LOG_SOURCE_LINUX].thread_started);
    TEST_ASSERT(state->sources[LOG_SOURCE_RTOS].thread_started);
    TEST_ASSERT(LogCollector_Start(_StoreEntry, &output) == -2);

    LogCollector_Stop();
    TEST_ASSERT(!LogCollector_IsRunning());
    TEST_ASSERT(!LogCollectorState_AnyThreadStarted());
    TEST_ASSERT(_test_thread_wait_count == LOG_SOURCE_MAX);
    TEST_ASSERT(_test_core_unregister_count == 2u);
    TEST_ASSERT(state->callback == NULL);
    TEST_ASSERT(state->user_data == NULL);

    LogCollector_Cleanup();
    return 0;
}

static int _TestThreadEntryCollectsUntilBridgeStops(void) {
    LogCollector_Config_t config;
    TestCollectorOutput_t output;
    unsigned              i;

    _ResetCollectorState();
    memset(&output, 0, sizeof(output));
    _FillCollectorConfig(&config);
    TEST_ASSERT(LogCollector_Init(&config) == 0);

    for (i = 0u; i < LOG_SOURCE_MAX; i++) {
        _test_thread_create_result[i] = 0;
    }
    _test_thread_run_entry_on_create = true;
    _test_rtt_running_calls_remaining = 1u;
    _SetCoreRecorderReadData(0u, "[15] threaded line\n");

    TEST_ASSERT(LogCollector_Start(_StoreEntry, &output) == 0);
    TEST_ASSERT(LogCollector_IsRunning());
    TEST_ASSERT(_test_thread_create_count == LOG_SOURCE_MAX);
    TEST_ASSERT(_test_sleep_count == 1u);
    TEST_ASSERT(output.count == 1u);
    TEST_ASSERT(LogEntry_GetTimestamp(output.entries[0]) == 15u);
    TEST_ASSERT(strcmp(LogEntry_GetContent(output.entries[0]), "threaded line") == 0);

    LogCollector_Stop();
    TEST_ASSERT(!LogCollector_IsRunning());
    TEST_ASSERT(_test_thread_wait_count == LOG_SOURCE_MAX);
    _DestroyStoredEntries(&output);

    LogCollector_Cleanup();
    return 0;
}

static int _TestStartFailureUnwindsStartedThreads(void) {
    LogCollector_Config_t config;
    LogCollector_State_t *state;
    TestCollectorOutput_t output;
    unsigned              i;

    _ResetCollectorState();
    memset(&output, 0, sizeof(output));
    _FillCollectorConfig(&config);
    TEST_ASSERT(LogCollector_Init(&config) == 0);

    for (i = 0u; i < LOG_SOURCE_MAX; i++) {
        _test_thread_create_result[i] = 0;
    }
    _test_thread_create_result[LOG_SOURCE_RTOS] = -1;

    TEST_ASSERT(LogCollector_Start(_StoreEntry, &output) == -2);
    state = LogCollectorState_Get();
    TEST_ASSERT(!LogCollector_IsRunning());
    TEST_ASSERT(!LogCollectorState_AnyThreadStarted());
    TEST_ASSERT(_test_thread_create_count == LOG_SOURCE_RTOS + 1u);
    TEST_ASSERT(_test_thread_wait_count == 1u);
    TEST_ASSERT(_test_core_register_count == 2u);
    TEST_ASSERT(_test_core_unregister_count == 2u);
    TEST_ASSERT(state->callback == NULL);
    TEST_ASSERT(state->user_data == NULL);

    LogCollector_Cleanup();
    return 0;
}

static int _TestPollTerminalResults(void) {
    LogCollector_Config_t config;
    TestCollectorOutput_t output;
    int                   result;

    _ResetCollectorState();
    memset(&output, 0, sizeof(output));
    _FillCollectorConfig(&config);
    config.rtos_channel = TEST_CORE_CHANNEL_COUNT;
    TEST_ASSERT(LogCollector_Init(&config) == 0);

    result = LogCollector_Poll(_StoreEntry, &output);
    TEST_ASSERT(result == LOG_COLLECT_RESULT_INVALID_RTT);
    TEST_ASSERT(LogCollector_HasFatalError());
    TEST_ASSERT(_test_core_register_count == 1u);
    TEST_ASSERT(_test_core_unregister_count == 1u);
    LogCollector_Cleanup();

    _ResetCollectorState();
    memset(&output, 0, sizeof(output));
    _FillCollectorConfig(&config);
    TEST_ASSERT(LogCollector_Init(&config) == 0);

    output.count = TEST_MAX_ENTRIES;
    _SetCoreRecorderReadData(0u, "[12] callback stop\n");
    result = LogCollector_Poll(_StoreEntry, &output);
    TEST_ASSERT(result == LOG_COLLECT_RESULT_STOP);
    TEST_ASSERT(!LogCollector_HasFatalError());
    TEST_ASSERT(_test_core_unregister_count == 2u);
    _DestroyStoredEntries(&output);

    LogCollector_Cleanup();
    return 0;
}

static int _ProcessTestLine(const char *line,
                            LogCollector_SourceState_t *source_state,
                            TestCollectorOutput_t *output) {
    return LogCollectorSource_ProcessLogLine(source_state,
                                             line,
                                             strlen(line),
                                             false,
                                             false,
                                             _StoreEntry,
                                             output);
}

static int _TestSourceInvalidInputContracts(void) {
    LogCollector_SourceState_t source_state;
    TestCollectorOutput_t      output;
    char                       pending[8];
    char                       line[] = "line";

    _ResetCollectorState();
    memset(&output, 0, sizeof(output));
    memset(&source_state, 0, sizeof(source_state));
    memset(pending, 0, sizeof(pending));

    TEST_ASSERT(LogCollectorSource_AppendPendingUntimedLine(NULL,
                                                            line,
                                                            strlen(line),
                                                            false,
                                                            false) == -1);
    TEST_ASSERT(LogCollectorSource_AppendPendingUntimedLine(&source_state,
                                                            line,
                                                            strlen(line),
                                                            false,
                                                            false) == -1);
    source_state.pending_untimed = pending;
    source_state.pending_untimed_size = sizeof(pending);
    TEST_ASSERT(LogCollectorSource_AppendPendingUntimedLine(&source_state,
                                                            NULL,
                                                            strlen(line),
                                                            false,
                                                            false) == -1);
    TEST_ASSERT(LogCollectorSource_AppendPendingUntimedLine(&source_state,
                                                            line,
                                                            0u,
                                                            false,
                                                            false) == -1);
    source_state.pending_untimed_size = LOG_COLLECTOR_PENDING_RECORD_OVERHEAD - 1u;
    TEST_ASSERT(LogCollectorSource_AppendPendingUntimedLine(&source_state,
                                                            line,
                                                            strlen(line),
                                                            false,
                                                            false) == -1);

    source_state.pending_untimed_size = sizeof(pending);
    TEST_ASSERT(LogCollectorSource_ProcessLogLine(&source_state,
                                                  line,
                                                  0u,
                                                  false,
                                                  false,
                                                  _StoreEntry,
                                                  &output) == -1);
    TEST_ASSERT(LogCollectorSource_ProcessLogLine(&source_state,
                                                  NULL,
                                                  1u,
                                                  false,
                                                  false,
                                                  _StoreEntry,
                                                  &output) == -1);
    TEST_ASSERT(LogCollectorSource_ProcessLogLine(&source_state,
                                                  line,
                                                  strlen(line),
                                                  false,
                                                  false,
                                                  NULL,
                                                  &output) == -1);
    source_state.pending_untimed = NULL;
    TEST_ASSERT(LogCollectorSource_ProcessLogLine(&source_state,
                                                  line,
                                                  strlen(line),
                                                  false,
                                                  false,
                                                  _StoreEntry,
                                                  &output) == -1);

    source_state.pending_untimed = pending;
    TEST_ASSERT(LogCollectorSource_ProcessBufferedLine(NULL,
                                                       line,
                                                       strlen(line),
                                                       false,
                                                       false,
                                                       _StoreEntry,
                                                       &output) == -1);
    TEST_ASSERT(LogCollectorSource_ProcessBufferedLine(&source_state,
                                                       NULL,
                                                       strlen(line),
                                                       false,
                                                       false,
                                                       _StoreEntry,
                                                       &output) == -1);
    TEST_ASSERT(LogCollectorSource_ProcessBufferedLine(&source_state,
                                                       line,
                                                       0u,
                                                       false,
                                                       false,
                                                       _StoreEntry,
                                                       &output) == -1);
    TEST_ASSERT(LogCollectorSource_ProcessBufferedLine(&source_state,
                                                       "   \t",
                                                       4u,
                                                       false,
                                                       false,
                                                       _StoreEntry,
                                                       &output) == -1);
    TEST_ASSERT(LogCollectorSource_FlushPendingLine(NULL,
                                                    _StoreEntry,
                                                    &output) == LOG_COLLECT_RESULT_INVALID_RTT);
    source_state.pending_untimed = NULL;
    TEST_ASSERT(LogCollectorSource_FlushPendingLine(&source_state,
                                                    _StoreEntry,
                                                    &output) == LOG_COLLECT_RESULT_INVALID_RTT);
    TEST_ASSERT(LogCollectorSource_FlushPendingUntimedFallback(&source_state,
                                                               NULL,
                                                               &output) == LOG_COLLECT_RESULT_INVALID_RTT);

    LogCollectorSource_Init(NULL, LOG_SOURCE_LINUX, 0u);
    LogCollectorSource_ResetForRun(NULL);
    LogCollectorSource_ReportUnflushedUntimedLines(NULL);
    _DestroyStoredEntries(&output);
    return 0;
}

static int _TestTimestampOnlyLineFlushesPendingUntimed(void) {
    LogCollector_SourceState_t source_state;
    TestCollectorOutput_t      output;
    char                       pending[128];
    int                        result;

    _ResetCollectorState();
    memset(&output, 0, sizeof(output));
    memset(&source_state, 0, sizeof(source_state));
    memset(pending, 0, sizeof(pending));

    source_state.source = LOG_SOURCE_LINUX;
    source_state.pending_untimed = pending;
    source_state.pending_untimed_size = sizeof(pending);

    result = _ProcessTestLine("pending banner", &source_state, &output);
    TEST_ASSERT(result == 0);
    TEST_ASSERT(output.count == 0u);
    TEST_ASSERT(source_state.pending_untimed_len > 0u);

    result = _ProcessTestLine("[700]", &source_state, &output);
    TEST_ASSERT(result == 1);
    TEST_ASSERT(output.count == 1u);
    TEST_ASSERT(LogEntry_GetTimestamp(output.entries[0]) == 700u);
    TEST_ASSERT(strcmp(LogEntry_GetContent(output.entries[0]), "pending banner") == 0);
    TEST_ASSERT(source_state.pending_untimed_len == 0u);
    TEST_ASSERT(source_state.last_timestamp == 700u);
    TEST_ASSERT(source_state.last_timestamp_valid);

    _DestroyStoredEntries(&output);
    return 0;
}

static int _TestLongSourceLineIsFragmented(void) {
    LogCollector_SourceState_t source_state;
    TestCollectorOutput_t      output;
    char                       pending[128];
    char                       content[LOG_ENTRY_MAX_CONTENT_LEN + 17u];
    int                        result;

    _ResetCollectorState();
    memset(&output, 0, sizeof(output));
    memset(&source_state, 0, sizeof(source_state));
    memset(pending, 0, sizeof(pending));
    memset(content, 'L', sizeof(content) - 1u);
    content[sizeof(content) - 1u] = '\0';

    source_state.source = LOG_SOURCE_LINUX;
    source_state.pending_untimed = pending;
    source_state.pending_untimed_size = sizeof(pending);
    source_state.last_timestamp = 900u;
    source_state.last_timestamp_valid = true;

    result = _ProcessTestLine(content, &source_state, &output);
    TEST_ASSERT(result == 2);
    TEST_ASSERT(output.count == 2u);
    TEST_ASSERT(LogEntry_GetTimestamp(output.entries[0]) == 900u);
    TEST_ASSERT(LogEntry_GetContentLen(output.entries[0]) == LOG_ENTRY_MAX_CONTENT_LEN);
    TEST_ASSERT(LogEntry_GetContentLen(output.entries[1]) ==
                sizeof(content) - 1u - LOG_ENTRY_MAX_CONTENT_LEN);
    TEST_ASSERT(!LogEntry_IsFragmentContinuation(output.entries[0]));
    TEST_ASSERT(LogEntry_FragmentContinues(output.entries[0]));
    TEST_ASSERT(LogEntry_IsFragmentContinuation(output.entries[1]));
    TEST_ASSERT(!LogEntry_FragmentContinues(output.entries[1]));
    TEST_ASSERT(source_state.sequence == 2u);

    _DestroyStoredEntries(&output);
    return 0;
}

static int _TestFlushPendingLineContracts(void) {
    LogCollector_SourceState_t source_state;
    TestCollectorOutput_t      output;
    char                       pending[128];
    int                        result;

    _ResetCollectorState();
    memset(&output, 0, sizeof(output));
    memset(&source_state, 0, sizeof(source_state));
    memset(pending, 0, sizeof(pending));

    source_state.source = LOG_SOURCE_RTOS;
    source_state.pending_untimed = pending;
    source_state.pending_untimed_size = sizeof(pending);
    source_state.line_buffer_len = sizeof(source_state.line_buffer);
    TEST_ASSERT(LogCollectorSource_FlushPendingLine(&source_state,
                                                    _StoreEntry,
                                                    &output) == LOG_COLLECT_RESULT_INVALID_RTT);

    source_state.line_buffer_len = strlen("[42] buffered");
    memcpy(source_state.line_buffer, "[42] buffered", source_state.line_buffer_len);
    result = LogCollectorSource_FlushPendingLine(&source_state, _StoreEntry, &output);
    TEST_ASSERT(result == 0);
    TEST_ASSERT(output.count == 1u);
    TEST_ASSERT(LogEntry_GetTimestamp(output.entries[0]) == 42u);
    TEST_ASSERT(strcmp(LogEntry_GetContent(output.entries[0]), "buffered") == 0);
    TEST_ASSERT(source_state.line_buffer_len == 0u);
    TEST_ASSERT(!source_state.fragmenting_line);
    _DestroyStoredEntries(&output);

    source_state.pending_untimed[0] = 'x';
    source_state.pending_untimed_len = 1u;
    source_state.last_timestamp_valid = false;
    TEST_ASSERT(LogCollectorSource_FlushPendingUntimedFallback(&source_state,
                                                               _StoreEntry,
                                                               &output) == LOG_COLLECT_RESULT_INVALID_RTT);

    return 0;
}

static int _TestPollFragmentsLongUntimedLineAcrossReads(void) {
    LogCollector_Config_t       config;
    LogCollector_State_t       *state;
    LogCollector_SourceState_t *source_state;
    TestCollectorOutput_t       output;
    char                        first_chunk[LOG_COLLECTOR_MAX_LINE_LEN];
    int                         result;

    _ResetCollectorState();
    memset(&output, 0, sizeof(output));
    memset(first_chunk, 'F', sizeof(first_chunk) - 1u);
    first_chunk[sizeof(first_chunk) - 1u] = '\0';
    _FillCollectorConfig(&config);
    TEST_ASSERT(LogCollector_Init(&config) == 0);

    state = LogCollectorState_Get();
    source_state = &state->sources[LOG_SOURCE_LINUX];

    _SetCoreRecorderReadData(0u, first_chunk);
    result = LogCollector_Poll(_StoreEntry, &output);
    TEST_ASSERT(result == 0);
    TEST_ASSERT(output.count == 0u);
    TEST_ASSERT(source_state->line_buffer_len == sizeof(first_chunk) - 1u);
    TEST_ASSERT(!source_state->fragmenting_line);

    _SetCoreRecorderReadData(0u, "tail\r\n");
    result = LogCollector_Poll(_StoreEntry, &output);
    TEST_ASSERT(result == 0);
    TEST_ASSERT(output.count == 0u);
    TEST_ASSERT(source_state->line_buffer_len == 0u);
    TEST_ASSERT(!source_state->fragmenting_line);
    TEST_ASSERT(source_state->pending_untimed_len > 0u);

    _SetCoreRecorderReadData(0u, "[33]\r\n");
    result = LogCollector_Poll(_StoreEntry, &output);
    TEST_ASSERT(result == 3);
    TEST_ASSERT(output.count == 3u);
    TEST_ASSERT(LogEntry_GetTimestamp(output.entries[0]) == 33u);
    TEST_ASSERT(LogEntry_GetTimestamp(output.entries[1]) == 33u);
    TEST_ASSERT(LogEntry_GetTimestamp(output.entries[2]) == 33u);
    TEST_ASSERT(LogEntry_GetContentLen(output.entries[0]) == LOG_ENTRY_MAX_CONTENT_LEN);
    TEST_ASSERT(LogEntry_GetContentLen(output.entries[1]) ==
                sizeof(first_chunk) - 1u - LOG_ENTRY_MAX_CONTENT_LEN);
    TEST_ASSERT(strcmp(LogEntry_GetContent(output.entries[2]), "tail") == 0);
    TEST_ASSERT(!LogEntry_IsFragmentContinuation(output.entries[0]));
    TEST_ASSERT(LogEntry_FragmentContinues(output.entries[0]));
    TEST_ASSERT(LogEntry_IsFragmentContinuation(output.entries[1]));
    TEST_ASSERT(LogEntry_FragmentContinues(output.entries[1]));
    TEST_ASSERT(LogEntry_IsFragmentContinuation(output.entries[2]));
    TEST_ASSERT(!LogEntry_FragmentContinues(output.entries[2]));
    TEST_ASSERT(source_state->pending_untimed_len == 0u);
    TEST_ASSERT(source_state->sequence == 3u);

    _DestroyStoredEntries(&output);
    LogCollector_Cleanup();
    return 0;
}

static int _TestUntimedLineDoesNotUseStaleSourceTimestamp(void) {
    TestCollectorOutput_t      output;
    LogCollector_SourceState_t source_state;
    char                       pending[128];
    int                        result;

    _ResetCollectorState();
    memset(&output, 0, sizeof(output));
    memset(&source_state, 0, sizeof(source_state));
    memset(pending, 0, sizeof(pending));

    source_state.source = LOG_SOURCE_RTOS;
    source_state.pending_untimed = pending;
    source_state.pending_untimed_size = sizeof(pending);
    source_state.last_timestamp = 100u;
    source_state.last_timestamp_valid = true;

    LogCollectorState_RecordObservedTimestamp(500u);

    result = _ProcessTestLine("RTOS boot banner without timestamp",
                              &source_state,
                              &output);

    TEST_ASSERT(result == 1);
    TEST_ASSERT(output.count == 1u);
    TEST_ASSERT(LogEntry_GetTimestamp(output.entries[0]) == 501u);
    TEST_ASSERT(source_state.last_timestamp == 501u);
    TEST_ASSERT(source_state.last_timestamp_valid);
    TEST_ASSERT(source_state.sequence == 1u);
    TEST_ASSERT(strcmp(LogEntry_GetContent(output.entries[0]),
                       "RTOS boot banner without timestamp") == 0);

    _DestroyStoredEntries(&output);
    return 0;
}

static int _TestUntimedLineKeepsCurrentTimestampWhenItIsNotStale(void) {
    TestCollectorOutput_t      output;
    LogCollector_SourceState_t source_state;
    char                       pending[128];
    int                        result;

    _ResetCollectorState();
    memset(&output, 0, sizeof(output));
    memset(&source_state, 0, sizeof(source_state));
    memset(pending, 0, sizeof(pending));

    source_state.source = LOG_SOURCE_LINUX;
    source_state.pending_untimed = pending;
    source_state.pending_untimed_size = sizeof(pending);
    source_state.last_timestamp = 600u;
    source_state.last_timestamp_valid = true;

    LogCollectorState_RecordObservedTimestamp(500u);

    result = _ProcessTestLine("same epoch line",
                              &source_state,
                              &output);

    TEST_ASSERT(result == 1);
    TEST_ASSERT(output.count == 1u);
    TEST_ASSERT(LogEntry_GetTimestamp(output.entries[0]) == 600u);
    TEST_ASSERT(source_state.last_timestamp == 600u);
    TEST_ASSERT(source_state.last_timestamp_valid);
    TEST_ASSERT(source_state.sequence == 1u);

    _DestroyStoredEntries(&output);
    return 0;
}

static int _TestLeadingUntimedLineWaitsForFirstTimestamp(void) {
    TestCollectorOutput_t      output;
    LogCollector_SourceState_t source_state;
    char                       pending[128];
    int                        result;

    _ResetCollectorState();
    memset(&output, 0, sizeof(output));
    memset(&source_state, 0, sizeof(source_state));
    memset(pending, 0, sizeof(pending));

    source_state.source = LOG_SOURCE_RTOS;
    source_state.pending_untimed = pending;
    source_state.pending_untimed_size = sizeof(pending);

    LogCollectorState_RecordObservedTimestamp(500u);

    result = _ProcessTestLine("leading banner",
                              &source_state,
                              &output);

    TEST_ASSERT(result == 0);
    TEST_ASSERT(output.count == 0u);
    TEST_ASSERT(source_state.pending_untimed_len > 0u);
    TEST_ASSERT(source_state.last_timestamp == 0u);
    TEST_ASSERT(!source_state.last_timestamp_valid);
    TEST_ASSERT(source_state.sequence == 0u);

    result = _ProcessTestLine("[100] first timestamped line",
                              &source_state,
                              &output);

    TEST_ASSERT(result == 2);
    TEST_ASSERT(output.count == 2u);
    TEST_ASSERT(LogEntry_GetTimestamp(output.entries[0]) == 100u);
    TEST_ASSERT(LogEntry_GetTimestamp(output.entries[1]) == 100u);
    TEST_ASSERT(strcmp(LogEntry_GetContent(output.entries[0]), "leading banner") == 0);
    TEST_ASSERT(strcmp(LogEntry_GetContent(output.entries[1]), "first timestamped line") == 0);
    TEST_ASSERT(source_state.pending_untimed_len == 0u);
    TEST_ASSERT(source_state.last_timestamp == 100u);
    TEST_ASSERT(source_state.last_timestamp_valid);
    TEST_ASSERT(source_state.sequence == 2u);

    _DestroyStoredEntries(&output);
    return 0;
}

static int _TestLeadingUntimedLineUsesFallbackOnFlush(void) {
    TestCollectorOutput_t      output;
    LogCollector_SourceState_t source_state;
    char                       pending[128];
    int                        result;

    _ResetCollectorState();
    memset(&output, 0, sizeof(output));
    memset(&source_state, 0, sizeof(source_state));
    memset(pending, 0, sizeof(pending));

    source_state.source = LOG_SOURCE_RTOS;
    source_state.pending_untimed = pending;
    source_state.pending_untimed_size = sizeof(pending);

    LogCollectorState_RecordObservedTimestamp(500u);

    result = _ProcessTestLine("leading banner",
                              &source_state,
                              &output);

    TEST_ASSERT(result == 0);
    TEST_ASSERT(output.count == 0u);
    TEST_ASSERT(source_state.pending_untimed_len > 0u);

    result = LogCollectorSource_FlushPendingUntimedFallback(&source_state,
                                                           _StoreEntry,
                                                           &output);

    TEST_ASSERT(result == 1);
    TEST_ASSERT(output.count == 1u);
    TEST_ASSERT(LogEntry_GetTimestamp(output.entries[0]) == 501u);
    TEST_ASSERT(strcmp(LogEntry_GetContent(output.entries[0]), "leading banner") == 0);
    TEST_ASSERT(source_state.pending_untimed_len == 0u);
    TEST_ASSERT(source_state.last_timestamp == 501u);
    TEST_ASSERT(source_state.last_timestamp_valid);
    TEST_ASSERT(source_state.sequence == 1u);

    _DestroyStoredEntries(&output);
    return 0;
}

static int _TestFatalStateResultContracts(void) {
    LogCollector_State_t *state;

    _ResetCollectorState();
    state = LogCollectorState_Get();
    state->running = true;
    TEST_ASSERT(!LogCollectorState_HasFatalError());
    TEST_ASSERT(LogCollectorState_GetFatalResult() == 0);

    TEST_ASSERT(LogCollectorState_EnterFatalStateWithResult(LOG_COLLECT_RESULT_STOP));
    TEST_ASSERT(LogCollectorState_HasFatalError());
    TEST_ASSERT(!state->running);
    TEST_ASSERT(LogCollectorState_GetFatalResult() == LOG_COLLECT_RESULT_STOP);
    TEST_ASSERT(!LogCollectorState_EnterFatalStateWithResult(LOG_COLLECT_RESULT_PENDING_OVERFLOW));
    TEST_ASSERT(LogCollectorState_GetFatalResult() == LOG_COLLECT_RESULT_STOP);

    _ResetCollectorState();
    LogCollectorState_SetFatalErrorResult(1234);
    TEST_ASSERT(LogCollectorState_HasFatalError());
    TEST_ASSERT(LogCollectorState_GetFatalResult() == LOG_COLLECT_RESULT_INVALID_RTT);

    _ResetCollectorState();
    LogCollectorState_ReportCallbackStop(LOG_SOURCE_RTOS);
    TEST_ASSERT(LogCollectorState_GetFatalResult() == LOG_COLLECT_RESULT_STOP);

    _ResetCollectorState();
    LogCollectorState_ReportRTTError(LOG_SOURCE_LINUX, NULL);
    TEST_ASSERT(LogCollectorState_GetFatalResult() == LOG_COLLECT_RESULT_INVALID_RTT);

    _ResetCollectorState();
    LogCollectorState_ReportFlushError(0);
    TEST_ASSERT(!LogCollectorState_HasFatalError());
    LogCollectorState_ReportFlushError(LOG_COLLECT_RESULT_PENDING_OVERFLOW);
    TEST_ASSERT(!LogCollectorState_HasFatalError());
    LogCollectorState_ReportFlushError(LOG_COLLECT_RESULT_STOP);
    TEST_ASSERT(LogCollectorState_GetFatalResult() == LOG_COLLECT_RESULT_STOP);

    _ResetCollectorState();
    LogCollectorState_RecordPendingOverflow(LOG_SOURCE_LINUX, 32u);
    TEST_ASSERT(LogCollectorState_GetFatalResult() == LOG_COLLECT_RESULT_PENDING_OVERFLOW);

    _ResetCollectorState();
    return 0;
}

/*********************************************************************
*
*       main()
*
*  Function description
*    Run LogCollector unit tests.
*
*  Return value
*    0  All tests passed.
*    1  A test failed.
*/
int main(void) {
    if (_TestDefaultPendingUntimedCapacity() != 0) {
        return 1;
    }
    if (_TestLifecycleWrapperBoundaryContracts() != 0) {
        return 1;
    }
    if (_TestPendingUntimedCapacityKeepsTrailingSentinel() != 0) {
        return 1;
    }
    if (_TestPollKeepsConsumersRegisteredBetweenCalls() != 0) {
        return 1;
    }
    if (_TestStartStopThreadLifecycle() != 0) {
        return 1;
    }
    if (_TestThreadEntryCollectsUntilBridgeStops() != 0) {
        return 1;
    }
    if (_TestStartFailureUnwindsStartedThreads() != 0) {
        return 1;
    }
    if (_TestPollTerminalResults() != 0) {
        return 1;
    }
    if (_TestSourceInvalidInputContracts() != 0) {
        return 1;
    }
    if (_TestTimestampOnlyLineFlushesPendingUntimed() != 0) {
        return 1;
    }
    if (_TestLongSourceLineIsFragmented() != 0) {
        return 1;
    }
    if (_TestFlushPendingLineContracts() != 0) {
        return 1;
    }
    if (_TestPollFragmentsLongUntimedLineAcrossReads() != 0) {
        return 1;
    }
    if (_TestUntimedLineDoesNotUseStaleSourceTimestamp() != 0) {
        return 1;
    }
    if (_TestUntimedLineKeepsCurrentTimestampWhenItIsNotStale() != 0) {
        return 1;
    }
    if (_TestLeadingUntimedLineWaitsForFirstTimestamp() != 0) {
        return 1;
    }
    if (_TestLeadingUntimedLineUsesFallbackOnFlush() != 0) {
        return 1;
    }
    if (_TestFatalStateResultContracts() != 0) {
        return 1;
    }
    return 0;
}

/*************************** End of file ****************************/
