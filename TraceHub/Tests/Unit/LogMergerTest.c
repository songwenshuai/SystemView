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
File    : LogMergerTest.c
Purpose : Unit checks for log merger failure and capacity semantics
---------------------------END-OF-HEADER------------------------------
*/

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "LogMerger.h"

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

/*********************************************************************
*
*       Types
*
**********************************************************************
*/

typedef struct {
    unsigned callback_count;
} TestOutputState_t;

/*********************************************************************
*
*       Static functions
*
**********************************************************************
*/

static void _FillConfig(LogMerger_Config_t *config, unsigned buffer_size,
                        unsigned flush_threshold, unsigned flush_timeout_ms,
                        bool linux_required, bool rtos_required) {
    memset(config, 0, sizeof(*config));
    config->buffer_size      = buffer_size;
    config->flush_threshold  = flush_threshold;
    config->flush_timeout_ms = flush_timeout_ms;
    config->required_source[LOG_SOURCE_LINUX] = linux_required;
    config->required_source[LOG_SOURCE_RTOS]  = rtos_required;
}

static int _DestroyAndContinue(LogEntry_t *entry, void *user_data) {
    TestOutputState_t *state;

    state = (TestOutputState_t *)user_data;
    if (state != NULL) {
        state->callback_count++;
    }
    LogEntry_Destroy(entry);
    return 0;
}

static int _DestroyAndStop(LogEntry_t *entry, void *user_data) {
    TestOutputState_t *state;

    state = (TestOutputState_t *)user_data;
    if (state != NULL) {
        state->callback_count++;
    }
    LogEntry_Destroy(entry);
    return -1;
}

static LogEntry_t *_CreateEntry(uint64_t timestamp, LogSource_t source) {
    static const char content[] = "entry";

    return LogEntry_Create(timestamp, source, content, sizeof(content) - 1u);
}

static int _TestOutputFailureDoesNotAdvanceDelivered(void) {
    LogMerger_Config_t config;
    TestOutputState_t  output_state;
    LogEntry_t        *entry;

    _FillConfig(&config, 2u, 1u, 0u, true, false);
    TEST_ASSERT(LogMerger_Init(&config) == 0);

    output_state.callback_count = 0u;
    entry = _CreateEntry(100u, LOG_SOURCE_LINUX);
    TEST_ASSERT(entry != NULL);
    TEST_ASSERT(LogMerger_Process(entry, _DestroyAndStop, &output_state) == -4);
    TEST_ASSERT(output_state.callback_count == 1u);

    entry = _CreateEntry(50u, LOG_SOURCE_LINUX);
    TEST_ASSERT(entry != NULL);
    TEST_ASSERT(LogMerger_Insert(entry) == 0);
    TEST_ASSERT(LogMerger_GetBufferedCount() == 1u);

    LogMerger_Cleanup();
    return 0;
}

static int _TestCapacityDoesNotForceUnreadyEntries(void) {
    LogMerger_Config_t config;
    TestOutputState_t  output_state;
    LogEntry_t        *entry;

    _FillConfig(&config, 1u, 1u, 0u, true, true);
    TEST_ASSERT(LogMerger_Init(&config) == 0);

    output_state.callback_count = 0u;
    entry = _CreateEntry(100u, LOG_SOURCE_LINUX);
    TEST_ASSERT(entry != NULL);
    TEST_ASSERT(LogMerger_Process(entry, _DestroyAndContinue, &output_state) == 0);
    TEST_ASSERT(output_state.callback_count == 0u);
    TEST_ASSERT(LogMerger_GetBufferedCount() == 1u);

    entry = _CreateEntry(101u, LOG_SOURCE_LINUX);
    TEST_ASSERT(entry != NULL);
    TEST_ASSERT(LogMerger_Process(entry, _DestroyAndContinue, &output_state) == -5);
    TEST_ASSERT(output_state.callback_count == 0u);
    TEST_ASSERT(LogMerger_GetBufferedCount() == 1u);

    LogMerger_Cleanup();
    return 0;
}

static int _TestCapacityWaitsForTimeoutReadyEntries(void) {
    LogMerger_Config_t config;
    TestOutputState_t  output_state;
    LogEntry_t        *entry;

    _FillConfig(&config, 1u, 1u, 1u, true, true);
    TEST_ASSERT(LogMerger_Init(&config) == 0);

    output_state.callback_count = 0u;
    entry = _CreateEntry(100u, LOG_SOURCE_LINUX);
    TEST_ASSERT(entry != NULL);
    TEST_ASSERT(LogMerger_Process(entry, _DestroyAndContinue, &output_state) == 0);
    TEST_ASSERT(output_state.callback_count == 0u);
    TEST_ASSERT(LogMerger_GetBufferedCount() == 1u);

    entry = _CreateEntry(101u, LOG_SOURCE_LINUX);
    TEST_ASSERT(entry != NULL);
    TEST_ASSERT(LogMerger_Process(entry, _DestroyAndContinue, &output_state) == 0);
    TEST_ASSERT(output_state.callback_count == 1u);
    TEST_ASSERT(LogMerger_GetBufferedCount() == 1u);

    LogMerger_Cleanup();
    return 0;
}

/*********************************************************************
*
*       main()
*/
int main(void) {
    if (_TestOutputFailureDoesNotAdvanceDelivered() != 0) {
        LogMerger_Cleanup();
        return 1;
    }
    if (_TestCapacityDoesNotForceUnreadyEntries() != 0) {
        LogMerger_Cleanup();
        return 1;
    }
    if (_TestCapacityWaitsForTimeoutReadyEntries() != 0) {
        LogMerger_Cleanup();
        return 1;
    }
    return 0;
}

/*************************** End of file ****************************/
