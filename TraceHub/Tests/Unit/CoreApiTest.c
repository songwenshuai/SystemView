/*********************************************************************
*                             CineLogic                              *
*                      RTT Trace and Debug Bridge                    *
**********************************************************************
----------------------------------------------------------------------
File    : CoreApiTest.c
Purpose : Unit checks for core utility APIs
---------------------------END-OF-HEADER------------------------------
*/

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ByteQueue.h"
#include "SYS.h"
#include "Utils.h"
#include "TestCommon.h"

typedef struct {
    volatile int value;
} TestThreadState_t;

static void _ThreadEntry(void *context) {
    TestThreadState_t *state;

    state = (TestThreadState_t *)context;
    if (state != NULL) {
        state->value = 7;
    }
}

static void _ExitThreadEntry(void *context) {
    TestThreadState_t *state;

    state = (TestThreadState_t *)context;
    if (state != NULL) {
        state->value = 9;
    }
    SYS_ExitThread(NULL);
}

static void _NoopThreadEntry(void *context) {
    (void)context;
}

static int _TestByteQueueLifecycle(void) {
    ByteQueue_t queue;
    char        storage[5];
    char        output[8];

    memset(&queue, 0, sizeof(queue));
    TEST_ASSERT(!ByteQueue_IsValid(NULL));
    ByteQueue_Init(&queue, NULL, sizeof(storage));
    TEST_ASSERT(!ByteQueue_IsValid(&queue));
    TEST_ASSERT(ByteQueue_GetCapacity(NULL) == 0u);
    TEST_ASSERT(ByteQueue_GetUsed(NULL) == 0u);

    ByteQueue_Init(&queue, storage, sizeof(storage));
    TEST_ASSERT(ByteQueue_IsValid(&queue));
    TEST_ASSERT(ByteQueue_GetCapacity(&queue) == sizeof(storage));
    TEST_ASSERT(ByteQueue_GetUsed(&queue) == 0u);

    TEST_ASSERT(ByteQueue_Write(&queue, "abc", 3u) == BYTE_QUEUE_WRITE_OK);
    TEST_ASSERT(ByteQueue_GetUsed(&queue) == 3u);
    TEST_ASSERT(ByteQueue_Read(&queue, output, 2u) == 2u);
    TEST_ASSERT(memcmp(output, "ab", 2u) == 0);
    TEST_ASSERT(ByteQueue_GetUsed(&queue) == 1u);

    TEST_ASSERT(ByteQueue_Write(&queue, "def", 3u) == BYTE_QUEUE_WRITE_OK);
    TEST_ASSERT(ByteQueue_Read(&queue, output, sizeof(output)) == 4u);
    TEST_ASSERT(memcmp(output, "cdef", 4u) == 0);
    TEST_ASSERT(ByteQueue_GetUsed(&queue) == 0u);

    TEST_ASSERT(ByteQueue_Write(&queue, "ab", 2u) == BYTE_QUEUE_WRITE_OK);
    TEST_ASSERT(ByteQueue_Write(&queue, "12345", 5u) == BYTE_QUEUE_WRITE_OVERFLOW_WRITTEN);
    TEST_ASSERT(ByteQueue_GetUsed(&queue) == 5u);
    TEST_ASSERT(ByteQueue_Read(&queue, output, sizeof(output)) == 5u);
    TEST_ASSERT(memcmp(output, "12345", 5u) == 0);

    TEST_ASSERT(ByteQueue_Write(&queue, "123456", 6u) == BYTE_QUEUE_WRITE_TOO_LARGE);
    TEST_ASSERT(ByteQueue_Write(&queue, NULL, 1u) == BYTE_QUEUE_WRITE_TOO_LARGE);
    TEST_ASSERT(ByteQueue_Write(&queue, NULL, 0u) == BYTE_QUEUE_WRITE_OK);

    ByteQueue_Clear(&queue);
    TEST_ASSERT(ByteQueue_GetUsed(&queue) == 0u);
    TEST_ASSERT(ByteQueue_Read(&queue, output, sizeof(output)) == 0u);
    return 0;
}

static int _TestSystemApiContracts(void) {
    SYS_Mutex          mutex;
    SYS_Thread         thread;
    SYS_Thread         detached_thread;
    TestThreadState_t  state;
    char               timestamp[32];
    uint64_t           before_us;
    uint64_t           after_us;
    unsigned           before_tick;
    unsigned           after_tick;

    TEST_ASSERT(SYS_MutexInit(NULL) == -1);
    TEST_ASSERT(SYS_MutexLock(NULL) == -1);
    TEST_ASSERT(SYS_MutexUnlock(NULL) == -1);
    TEST_ASSERT(SYS_MutexDestroy(NULL) == -1);

    TEST_ASSERT(SYS_MutexInit(&mutex) == 0);
    TEST_ASSERT(SYS_MutexLock(&mutex) == 0);
    TEST_ASSERT(SYS_MutexUnlock(&mutex) == 0);
    TEST_ASSERT(SYS_MutexDestroy(&mutex) == 0);

    TEST_ASSERT(SYS_createThread(NULL, NULL, &thread) == -1);
    TEST_ASSERT(SYS_createThread(_ThreadEntry, NULL, NULL) == -1);
    state.value = 0;
    TEST_ASSERT(SYS_createThread(_ThreadEntry, &state, &thread) == 1);
    SYS_WaitThreadTerm(thread);
    TEST_ASSERT(state.value == 7);

    state.value = 0;
    TEST_ASSERT(SYS_createThread(_ExitThreadEntry, &state, &thread) == 1);
    SYS_WaitThreadTerm(thread);
    TEST_ASSERT(state.value == 9);

    TEST_ASSERT(SYS_createThread(_NoopThreadEntry, NULL, &detached_thread) == 1);
    SYS_destroyThread(detached_thread);

    before_us = SYS_GetMonotonicTimeUs();
    SYS_Sleep(0u);
    after_us = SYS_GetMonotonicTimeUs();
    TEST_ASSERT(after_us >= before_us);
    before_tick = SYS_GetTickCount();
    after_tick = SYS_GetTickCount();
    TEST_ASSERT((unsigned)(after_tick - before_tick) < 1000u);

    memset(timestamp, 0, sizeof(timestamp));
    SYS_GetTimestampStr(timestamp, sizeof(timestamp));
    TEST_ASSERT(strlen(timestamp) >= strlen("00:00:00.000"));
    TEST_ASSERT(timestamp[2] == ':');
    TEST_ASSERT(timestamp[5] == ':');
    TEST_ASSERT(timestamp[8] == '.');

    SYS_GetTimestampStr(NULL, sizeof(timestamp));
    SYS_GetTimestampStr(timestamp, 0u);
    return 0;
}

static int _TestRealPathUtility(void) {
    char *resolved;
    char *missing;

    resolved = UTILS_LRealPath(".");
    TEST_ASSERT(resolved != NULL);
    TEST_ASSERT(resolved[0] != '\0');
    free(resolved);

    missing = UTILS_LRealPath("tracehub_unit_missing_path");
    TEST_ASSERT(missing != NULL);
    TEST_ASSERT(strcmp(missing, "tracehub_unit_missing_path") == 0 ||
                missing[0] != '\0');
    free(missing);
    return 0;
}

int main(void) {
    TEST_RUN(_TestByteQueueLifecycle);
    TEST_RUN(_TestSystemApiContracts);
    TEST_RUN(_TestRealPathUtility);
    return 0;
}

/*************************** End of file ****************************/
