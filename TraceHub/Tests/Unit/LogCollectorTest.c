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
#include <string.h>

#include "CoreLogRecorder.h"
#include "RTTBridge.h"
#include "SYS.h"

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

#define TEST_MAX_ENTRIES 8u

/*********************************************************************
*
*       Runtime dependency stubs
*
**********************************************************************
*/

int CoreLogRecorder_RegisterConsumer(unsigned channel) {
    (void)channel;
    return 0;
}

void CoreLogRecorder_UnregisterConsumer(unsigned channel) {
    (void)channel;
}

int CoreLogRecorder_ReadChannel(unsigned channel, void *buffer, size_t buffer_size) {
    (void)channel;
    (void)buffer;
    (void)buffer_size;
    return 0;
}

bool RTTBridge_IsRunning(void) {
    return false;
}

int SYS_createThread(void (*threadEntry)(void *), void *context, SYS_Thread *pRetTid) {
    (void)threadEntry;
    (void)context;
    (void)pRetTid;
    return -1;
}

void SYS_destroyThread(SYS_Thread handle) {
    (void)handle;
}

void SYS_ExitThread(void *status) {
    (void)status;
}

void SYS_WaitThreadTerm(SYS_Thread pRetTid) {
    (void)pRetTid;
}

unsigned SYS_GetTickCount(void) {
    return 0u;
}

uint64_t SYS_GetMonotonicTimeUs(void) {
    return 0u;
}

void SYS_Sleep(unsigned ms) {
    (void)ms;
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

#include "LogCollector.c"

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
    memset(&_collector_state, 0, sizeof(_collector_state));
}

static int _ProcessTestLine(const char *line,
                            LogSource_t source,
                            uint64_t *last_timestamp,
                            bool *timestamp_valid,
                            char *pending_buffer,
                            size_t pending_size,
                            size_t *pending_len,
                            uint64_t *sequence,
                            TestCollectorOutput_t *output) {
    return _ProcessLogLine(line,
                           strlen(line),
                           source,
                           last_timestamp,
                           timestamp_valid,
                           pending_buffer,
                           pending_size,
                           pending_len,
                           sequence,
                           false,
                           false,
                           _StoreEntry,
                           output);
}

static int _TestUntimedLineDoesNotUseStaleSourceTimestamp(void) {
    TestCollectorOutput_t output;
    char                  pending[128];
    size_t                pending_len;
    uint64_t              source_timestamp;
    uint64_t              sequence;
    bool                  timestamp_valid;
    int                   result;

    _ResetCollectorState();
    memset(&output, 0, sizeof(output));
    memset(pending, 0, sizeof(pending));

    pending_len = 0u;
    source_timestamp = 100u;
    sequence = 0u;
    timestamp_valid = true;

    _Collector_RecordObservedTimestamp(500u);

    result = _ProcessTestLine("RTOS R5 boot banner without timestamp",
                              LOG_SOURCE_RTOS,
                              &source_timestamp,
                              &timestamp_valid,
                              pending,
                              sizeof(pending),
                              &pending_len,
                              &sequence,
                              &output);

    TEST_ASSERT(result == 1);
    TEST_ASSERT(output.count == 1u);
    TEST_ASSERT(LogEntry_GetTimestamp(output.entries[0]) == 501u);
    TEST_ASSERT(source_timestamp == 501u);
    TEST_ASSERT(timestamp_valid);
    TEST_ASSERT(sequence == 1u);
    TEST_ASSERT(strcmp(LogEntry_GetContent(output.entries[0]),
                       "RTOS R5 boot banner without timestamp") == 0);

    _DestroyStoredEntries(&output);
    return 0;
}

static int _TestUntimedLineKeepsCurrentTimestampWhenItIsNotStale(void) {
    TestCollectorOutput_t output;
    char                  pending[128];
    size_t                pending_len;
    uint64_t              source_timestamp;
    uint64_t              sequence;
    bool                  timestamp_valid;
    int                   result;

    _ResetCollectorState();
    memset(&output, 0, sizeof(output));
    memset(pending, 0, sizeof(pending));

    pending_len = 0u;
    source_timestamp = 600u;
    sequence = 0u;
    timestamp_valid = true;

    _Collector_RecordObservedTimestamp(500u);

    result = _ProcessTestLine("same epoch line",
                              LOG_SOURCE_LINUX,
                              &source_timestamp,
                              &timestamp_valid,
                              pending,
                              sizeof(pending),
                              &pending_len,
                              &sequence,
                              &output);

    TEST_ASSERT(result == 1);
    TEST_ASSERT(output.count == 1u);
    TEST_ASSERT(LogEntry_GetTimestamp(output.entries[0]) == 600u);
    TEST_ASSERT(source_timestamp == 600u);
    TEST_ASSERT(timestamp_valid);
    TEST_ASSERT(sequence == 1u);

    _DestroyStoredEntries(&output);
    return 0;
}

static int _TestLeadingUntimedLineWaitsForFirstTimestamp(void) {
    TestCollectorOutput_t output;
    char                  pending[128];
    size_t                pending_len;
    uint64_t              source_timestamp;
    uint64_t              sequence;
    bool                  timestamp_valid;
    int                   result;

    _ResetCollectorState();
    memset(&output, 0, sizeof(output));
    memset(pending, 0, sizeof(pending));

    pending_len = 0u;
    source_timestamp = 0u;
    sequence = 0u;
    timestamp_valid = false;

    _Collector_RecordObservedTimestamp(500u);

    result = _ProcessTestLine("leading banner",
                              LOG_SOURCE_RTOS,
                              &source_timestamp,
                              &timestamp_valid,
                              pending,
                              sizeof(pending),
                              &pending_len,
                              &sequence,
                              &output);

    TEST_ASSERT(result == 0);
    TEST_ASSERT(output.count == 0u);
    TEST_ASSERT(pending_len > 0u);
    TEST_ASSERT(!timestamp_valid);

    result = _ProcessTestLine("[600] first timestamped line",
                              LOG_SOURCE_RTOS,
                              &source_timestamp,
                              &timestamp_valid,
                              pending,
                              sizeof(pending),
                              &pending_len,
                              &sequence,
                              &output);

    TEST_ASSERT(result == 2);
    TEST_ASSERT(output.count == 2u);
    TEST_ASSERT(LogEntry_GetTimestamp(output.entries[0]) == 600u);
    TEST_ASSERT(LogEntry_GetTimestamp(output.entries[1]) == 600u);
    TEST_ASSERT(strcmp(LogEntry_GetContent(output.entries[0]), "leading banner") == 0);
    TEST_ASSERT(strcmp(LogEntry_GetContent(output.entries[1]), "first timestamped line") == 0);
    TEST_ASSERT(pending_len == 0u);
    TEST_ASSERT(source_timestamp == 600u);
    TEST_ASSERT(timestamp_valid);
    TEST_ASSERT(sequence == 2u);

    _DestroyStoredEntries(&output);
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
    if (_TestUntimedLineDoesNotUseStaleSourceTimestamp() != 0) {
        return 1;
    }
    if (_TestUntimedLineKeepsCurrentTimestampWhenItIsNotStale() != 0) {
        return 1;
    }
    if (_TestLeadingUntimedLineWaitsForFirstTimestamp() != 0) {
        return 1;
    }
    return 0;
}

/*************************** End of file ****************************/
