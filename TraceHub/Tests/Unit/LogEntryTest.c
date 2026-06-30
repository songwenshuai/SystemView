/*********************************************************************
*                             CineLogic                              *
*                      RTT Trace and Debug Bridge                    *
**********************************************************************
----------------------------------------------------------------------
File    : LogEntryTest.c
Purpose : Unit checks for log entry public APIs
---------------------------END-OF-HEADER------------------------------
*/

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "LogEntry.h"
#include "TestCommon.h"

static int _TestCreateCloneAndGetters(void) {
    LogEntry_t *entry;
    LogEntry_t *clone;

    entry = LogEntry_CreateEx(42u,
                              LOG_SOURCE_RTOS,
                              9u,
                              true,
                              true,
                              "payload-tail",
                              7u);
    TEST_ASSERT(entry != NULL);
    TEST_ASSERT(LogEntry_IsValid(entry));
    TEST_ASSERT(LogEntry_GetTimestamp(entry) == 42u);
    TEST_ASSERT(LogEntry_GetSource(entry) == LOG_SOURCE_RTOS);
    TEST_ASSERT(LogEntry_GetSequence(entry) == 9u);
    TEST_ASSERT(LogEntry_GetContentLen(entry) == 7u);
    TEST_ASSERT(strcmp(LogEntry_GetContent(entry), "payload") == 0);
    TEST_ASSERT(LogEntry_IsFragmentContinuation(entry));
    TEST_ASSERT(LogEntry_FragmentContinues(entry));

    clone = LogEntry_Clone(entry);
    TEST_ASSERT(clone != NULL);
    TEST_ASSERT(clone != entry);
    TEST_ASSERT(LogEntry_GetContent(clone) != LogEntry_GetContent(entry));
    TEST_ASSERT(LogEntry_Compare(entry, clone) == 0);

    LogEntry_Destroy(entry);
    TEST_ASSERT(LogEntry_IsValid(clone));
    LogEntry_Destroy(clone);
    LogEntry_Destroy(NULL);
    return 0;
}

static int _TestRejectInvalidCreateInputs(void) {
    char content[LOG_ENTRY_MAX_CONTENT_LEN + 1u];

    memset(content, 'A', sizeof(content));
    TEST_ASSERT(LogEntry_Create(0u, LOG_SOURCE_LINUX, NULL, 1u) == NULL);
    TEST_ASSERT(LogEntry_Create(0u, LOG_SOURCE_LINUX, "x", 0u) == NULL);
    TEST_ASSERT(LogEntry_Create(0u, LOG_SOURCE_MAX, "x", 1u) == NULL);
    TEST_ASSERT(LogEntry_Create(0u,
                                LOG_SOURCE_LINUX,
                                content,
                                sizeof(content)) == NULL);
    return 0;
}

static int _TestInvalidEntryAccessors(void) {
    LogEntry_t invalid;

    memset(&invalid, 0, sizeof(invalid));
    invalid.valid = false;
    invalid.source = LOG_SOURCE_RTOS;

    TEST_ASSERT(LogEntry_GetTimestamp(NULL) == 0u);
    TEST_ASSERT(LogEntry_GetSource(NULL) == LOG_SOURCE_LINUX);
    TEST_ASSERT(LogEntry_GetSequence(NULL) == 0u);
    TEST_ASSERT(LogEntry_GetContent(NULL) == NULL);
    TEST_ASSERT(LogEntry_GetContentLen(NULL) == 0u);
    TEST_ASSERT(!LogEntry_IsFragmentContinuation(NULL));
    TEST_ASSERT(!LogEntry_FragmentContinues(NULL));
    TEST_ASSERT(!LogEntry_IsValid(NULL));
    TEST_ASSERT(LogEntry_Clone(NULL) == NULL);

    TEST_ASSERT(LogEntry_GetTimestamp(&invalid) == 0u);
    TEST_ASSERT(LogEntry_GetSource(&invalid) == LOG_SOURCE_LINUX);
    TEST_ASSERT(LogEntry_GetSequence(&invalid) == 0u);
    TEST_ASSERT(LogEntry_GetContent(&invalid) == NULL);
    TEST_ASSERT(LogEntry_GetContentLen(&invalid) == 0u);
    TEST_ASSERT(!LogEntry_IsValid(&invalid));
    TEST_ASSERT(LogEntry_Clone(&invalid) == NULL);
    return 0;
}

static int _TestCompareTotalOrdering(void) {
    LogEntry_t *early;
    LogEntry_t *late;
    LogEntry_t *linux_entry;
    LogEntry_t *rtos_entry;
    LogEntry_t *seq_low;
    LogEntry_t *seq_high;
    LogEntry_t  invalid;

    early = LogEntry_Create(1u, LOG_SOURCE_RTOS, "a", 1u);
    late = LogEntry_Create(2u, LOG_SOURCE_LINUX, "b", 1u);
    linux_entry = LogEntry_Create(5u, LOG_SOURCE_LINUX, "c", 1u);
    rtos_entry = LogEntry_Create(5u, LOG_SOURCE_RTOS, "d", 1u);
    seq_low = LogEntry_CreateEx(9u, LOG_SOURCE_LINUX, 1u, false, false, "e", 1u);
    seq_high = LogEntry_CreateEx(9u, LOG_SOURCE_LINUX, 2u, false, false, "f", 1u);
    memset(&invalid, 0, sizeof(invalid));

    TEST_ASSERT(early != NULL);
    TEST_ASSERT(late != NULL);
    TEST_ASSERT(linux_entry != NULL);
    TEST_ASSERT(rtos_entry != NULL);
    TEST_ASSERT(seq_low != NULL);
    TEST_ASSERT(seq_high != NULL);

    TEST_ASSERT(LogEntry_Compare(early, late) < 0);
    TEST_ASSERT(LogEntry_Compare(late, early) > 0);
    TEST_ASSERT(LogEntry_Compare(linux_entry, rtos_entry) < 0);
    TEST_ASSERT(LogEntry_Compare(rtos_entry, linux_entry) > 0);
    TEST_ASSERT(LogEntry_Compare(seq_low, seq_high) < 0);
    TEST_ASSERT(LogEntry_Compare(seq_high, seq_low) > 0);
    TEST_ASSERT(LogEntry_Compare(NULL, NULL) == 0);
    TEST_ASSERT(LogEntry_Compare(NULL, early) > 0);
    TEST_ASSERT(LogEntry_Compare(early, NULL) < 0);
    TEST_ASSERT(LogEntry_Compare(&invalid, early) > 0);

    LogEntry_Destroy(early);
    LogEntry_Destroy(late);
    LogEntry_Destroy(linux_entry);
    LogEntry_Destroy(rtos_entry);
    LogEntry_Destroy(seq_low);
    LogEntry_Destroy(seq_high);
    return 0;
}

int main(void) {
    TEST_RUN(_TestCreateCloneAndGetters);
    TEST_RUN(_TestRejectInvalidCreateInputs);
    TEST_RUN(_TestInvalidEntryAccessors);
    TEST_RUN(_TestCompareTotalOrdering);
    return 0;
}

/*************************** End of file ****************************/
