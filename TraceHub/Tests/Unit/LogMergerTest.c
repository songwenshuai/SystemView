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

#include "Log.h"
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
                        bool linux_required, bool rtos_required) {
    memset(config, 0, sizeof(*config));
    config->buffer_size      = buffer_size;
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

static LogEntry_t *_CreateSequencedEntry(uint64_t timestamp, LogSource_t source,
                                         uint64_t sequence) {
    static const char content[] = "entry";

    return LogEntry_CreateEx(timestamp,
                             source,
                             sequence,
                             false,
                             false,
                             content,
                             sizeof(content) - 1u);
}

static int _ReadTmpFile(FILE *file, char *buffer, size_t buffer_size) {
    size_t len;

    if (file == NULL || buffer == NULL || buffer_size == 0u) {
        return -1;
    }
    if (fflush(file) != 0) {
        return -1;
    }
    rewind(file);
    len = fread(buffer, 1u, buffer_size - 1u, file);
    if (ferror(file)) {
        return -1;
    }
    buffer[len] = '\0';
    return 0;
}

static int _TestCleanTextWriterStripsTerminalControls(void) {
    static const char part_one[] = "A\033[31";
    static const char part_two[] =
        "mB\033[0m\n"
        "C\rD\tE\a"
        "\033]0;title\aF"
        "\033OAG"
        "\033Ppayload\033\\H"
        "\x9B""32mI"
        "\x9D""0;c1-title\x9C""J"
        "\x90""payload\x9C""K"
        "\x8E""XL"
        "\xC4\x80""M"
        "\xE2\x82\xAC""N"
        "\xC2\x9B""31mO";
    static const char split_utf8_one[] = "\xE2";
    static const char split_utf8_two[] = "\x82\xAC""P";
    LOG_TextCleanState_t state;
    FILE                *file;
    char                 buffer[96];
    size_t               clean_len;
    size_t               total_clean_len;
    int                  result;

    file = tmpfile();
    TEST_ASSERT(file != NULL);

    LOG_TextCleanStateInit(&state);
    total_clean_len = 0u;
    clean_len = 0u;
    TEST_ASSERT(LOG_WriteCleanTextToFileEx(file,
                                           part_one,
                                           strlen(part_one),
                                           &state,
                                           &clean_len) == 0);
    TEST_ASSERT(clean_len == 1u);
    total_clean_len += clean_len;

    clean_len = 0u;
    TEST_ASSERT(LOG_WriteCleanTextToFileEx(file,
                                           part_two,
                                           strlen(part_two),
                                           &state,
                                           &clean_len) == 0);
    TEST_ASSERT(clean_len == 21u);
    total_clean_len += clean_len;

    clean_len = 0u;
    TEST_ASSERT(LOG_WriteCleanTextToFileEx(file,
                                           split_utf8_one,
                                           strlen(split_utf8_one),
                                           &state,
                                           &clean_len) == 0);
    TEST_ASSERT(clean_len == 0u);
    total_clean_len += clean_len;

    clean_len = 0u;
    TEST_ASSERT(LOG_WriteCleanTextToFileEx(file,
                                           split_utf8_two,
                                           strlen(split_utf8_two),
                                           &state,
                                           &clean_len) == 0);
    TEST_ASSERT(clean_len == 4u);
    total_clean_len += clean_len;

    result = _ReadTmpFile(file, buffer, sizeof(buffer));
    TEST_ASSERT(fclose(file) == 0);
    TEST_ASSERT(result == 0);
    TEST_ASSERT(total_clean_len == strlen(buffer));
    TEST_ASSERT(strcmp(buffer,
                       "AB\nCD\tEFGHIJKL"
                       "\xC4\x80""M"
                       "\xE2\x82\xAC""NO"
                       "\xE2\x82\xAC""P") == 0);
    return 0;
}

static int _TestSwimLaneFileWriterStripsTerminalControls(void) {
    LOG_TextCleanState_t state;
    FILE                *file;
    char                 buffer[128];
    int                  result;

    file = tmpfile();
    TEST_ASSERT(file != NULL);

    LOG_TextCleanStateInit(&state);
    TEST_ASSERT(LOG_SwimLaneLogToFile(file,
                                      42u,
                                      "LINUX",
                                      "\033[32mready\n\033[0m",
                                      &state) == 0);

    result = _ReadTmpFile(file, buffer, sizeof(buffer));
    TEST_ASSERT(fclose(file) == 0);
    TEST_ASSERT(result == 0);
    TEST_ASSERT(strcmp(buffer, "[42] [LINUX] ready\n") == 0);
    return 0;
}

static int _TestWriteEntryContracts(void) {
    LogMerger_Config_t config;
    LogEntry_t        *entry;

    TEST_ASSERT(LogMerger_WriteEntry(NULL) == -1);
    TEST_ASSERT(LogMerger_WriteEntry((const LogEntry_t *)1) == -1);
    TEST_ASSERT(!LogMerger_HasFileError());

    _FillConfig(&config, LOG_MERGER_DEFAULT_BUFFER_SIZE, true, false);
    TEST_ASSERT(LogMerger_Init(&config) == 0);
    entry = _CreateEntry(42u, LOG_SOURCE_LINUX);
    TEST_ASSERT(entry != NULL);
    TEST_ASSERT(LogMerger_WriteEntry(entry) == 0);
    TEST_ASSERT(!LogMerger_HasFileError());
    LogEntry_Destroy(entry);
    LogMerger_Cleanup();

    _FillConfig(&config, LOG_MERGER_DEFAULT_BUFFER_SIZE, true, false);
    config.log_enabled = true;
    config.log_prefix = "tracehub_unit_merger";
    TEST_ASSERT(LogMerger_Init(&config) == 0);
    entry = _CreateEntry(43u, LOG_SOURCE_LINUX);
    TEST_ASSERT(entry != NULL);
    TEST_ASSERT(LogMerger_WriteEntry(entry) == 0);
    TEST_ASSERT(!LogMerger_HasFileError());
    LogEntry_Destroy(entry);
    LogMerger_Cleanup();
    return 0;
}

static int _TestOutputFailureDoesNotAdvanceDelivered(void) {
    LogMerger_Config_t config;
    TestOutputState_t  output_state;
    LogEntry_t        *entry;

    _FillConfig(&config, LOG_MERGER_DEFAULT_BUFFER_SIZE, true, false);
    TEST_ASSERT(LogMerger_Init(&config) == 0);

    output_state.callback_count = 0u;
    entry = _CreateEntry(100u, LOG_SOURCE_LINUX);
    TEST_ASSERT(entry != NULL);
    TEST_ASSERT(LogMerger_Insert(entry) == 0);
    TEST_ASSERT(LogMerger_Flush(_DestroyAndStop, &output_state) == -4);
    TEST_ASSERT(output_state.callback_count == 1u);

    entry = _CreateEntry(50u, LOG_SOURCE_LINUX);
    TEST_ASSERT(entry != NULL);
    TEST_ASSERT(LogMerger_Insert(entry) == 0);
    TEST_ASSERT(LogMerger_GetBufferedCount() == 1u);

    LogMerger_Cleanup();
    return 0;
}

static int _TestDefaultThresholdFlushesReadyEntries(void) {
    LogMerger_Config_t config;
    TestOutputState_t  output_state;
    LogEntry_t        *entry;
    unsigned           i;

    _FillConfig(&config, LOG_MERGER_DEFAULT_BUFFER_SIZE, true, false);
    TEST_ASSERT(LogMerger_Init(&config) == 0);

    output_state.callback_count = 0u;
    for (i = 0u; i < LOG_MERGER_DEFAULT_FLUSH_THRESHOLD; i++) {
        entry = _CreateEntry(100u + i, LOG_SOURCE_LINUX);
        TEST_ASSERT(entry != NULL);
        TEST_ASSERT(LogMerger_Process(entry, _DestroyAndContinue, &output_state) == 0);
    }
    TEST_ASSERT(output_state.callback_count == LOG_MERGER_DEFAULT_FLUSH_THRESHOLD);
    TEST_ASSERT(LogMerger_GetBufferedCount() == 0u);

    LogMerger_Cleanup();
    return 0;
}

static int _TestFlushReadyDoesNotForceUnreadyEntries(void) {
    LogMerger_Config_t config;
    TestOutputState_t  output_state;
    LogEntry_t        *entry;

    _FillConfig(&config, LOG_MERGER_DEFAULT_FLUSH_THRESHOLD, true, true);
    TEST_ASSERT(LogMerger_Init(&config) == 0);

    output_state.callback_count = 0u;
    entry = _CreateEntry(100u, LOG_SOURCE_LINUX);
    TEST_ASSERT(entry != NULL);
    TEST_ASSERT(LogMerger_Insert(entry) == 0);
    TEST_ASSERT(LogMerger_FlushReady(_DestroyAndContinue, &output_state) == 0);
    TEST_ASSERT(output_state.callback_count == 0u);
    TEST_ASSERT(LogMerger_GetBufferedCount() == 1u);

    LogMerger_Cleanup();
    return 0;
}

static int _TestCapacityWaitsForTimeoutReadyEntries(void) {
    LogMerger_Config_t config;
    TestOutputState_t  output_state;
    LogEntry_t        *entry;

    _FillConfig(&config, LOG_MERGER_DEFAULT_FLUSH_THRESHOLD, true, true);
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
    TEST_ASSERT(output_state.callback_count == 0u);
    TEST_ASSERT(LogMerger_GetBufferedCount() == LOG_MERGER_DEFAULT_FLUSH_THRESHOLD);

    entry = _CreateEntry(102u, LOG_SOURCE_LINUX);
    TEST_ASSERT(entry != NULL);
    TEST_ASSERT(LogMerger_Process(entry, _DestroyAndContinue, &output_state) == 0);
    TEST_ASSERT(output_state.callback_count == LOG_MERGER_DEFAULT_FLUSH_THRESHOLD);
    TEST_ASSERT(LogMerger_GetBufferedCount() == 1u);

    LogMerger_Cleanup();
    return 0;
}

static int _TestEqualTimestampBeforeDeliveredIsRejected(void) {
    LogMerger_Config_t config;
    TestOutputState_t  output_state;
    LogEntry_t        *entry;

    _FillConfig(&config, LOG_MERGER_DEFAULT_BUFFER_SIZE, true, true);
    TEST_ASSERT(LogMerger_Init(&config) == 0);

    output_state.callback_count = 0u;
    entry = _CreateEntry(100u, LOG_SOURCE_RTOS);
    TEST_ASSERT(entry != NULL);
    TEST_ASSERT(LogMerger_Insert(entry) == 0);
    TEST_ASSERT(LogMerger_Flush(_DestroyAndContinue, &output_state) == 1);
    TEST_ASSERT(output_state.callback_count == 1u);

    entry = _CreateEntry(100u, LOG_SOURCE_LINUX);
    TEST_ASSERT(entry != NULL);
    TEST_ASSERT(LogMerger_Insert(entry) == -4);

    LogMerger_Cleanup();
    return 0;
}

static int _TestEqualOrderingKeyAfterDeliveredIsRejected(void) {
    LogMerger_Config_t config;
    TestOutputState_t  output_state;
    LogEntry_t        *entry;

    _FillConfig(&config, LOG_MERGER_DEFAULT_BUFFER_SIZE, true, true);
    TEST_ASSERT(LogMerger_Init(&config) == 0);

    output_state.callback_count = 0u;
    entry = _CreateSequencedEntry(100u, LOG_SOURCE_LINUX, 7u);
    TEST_ASSERT(entry != NULL);
    TEST_ASSERT(LogMerger_Insert(entry) == 0);
    TEST_ASSERT(LogMerger_Flush(_DestroyAndContinue, &output_state) == 1);
    TEST_ASSERT(output_state.callback_count == 1u);

    entry = _CreateSequencedEntry(100u, LOG_SOURCE_LINUX, 7u);
    TEST_ASSERT(entry != NULL);
    TEST_ASSERT(LogMerger_Insert(entry) == -4);

    entry = _CreateSequencedEntry(100u, LOG_SOURCE_LINUX, 8u);
    TEST_ASSERT(entry != NULL);
    TEST_ASSERT(LogMerger_Insert(entry) == 0);

    LogMerger_Cleanup();
    return 0;
}

/*********************************************************************
*
*       main()
*
*  Function description
*    Run LogMerger unit tests.
*
*  Return value
*    0  All tests passed.
*    1  A test failed.
*/
int main(void) {
    if (_TestCleanTextWriterStripsTerminalControls() != 0) {
        return 1;
    }
    if (_TestSwimLaneFileWriterStripsTerminalControls() != 0) {
        return 1;
    }
    if (_TestWriteEntryContracts() != 0) {
        LogMerger_Cleanup();
        return 1;
    }
    if (_TestOutputFailureDoesNotAdvanceDelivered() != 0) {
        LogMerger_Cleanup();
        return 1;
    }
    if (_TestDefaultThresholdFlushesReadyEntries() != 0) {
        LogMerger_Cleanup();
        return 1;
    }
    if (_TestFlushReadyDoesNotForceUnreadyEntries() != 0) {
        LogMerger_Cleanup();
        return 1;
    }
    if (_TestCapacityWaitsForTimeoutReadyEntries() != 0) {
        LogMerger_Cleanup();
        return 1;
    }
    if (_TestEqualTimestampBeforeDeliveredIsRejected() != 0) {
        LogMerger_Cleanup();
        return 1;
    }
    if (_TestEqualOrderingKeyAfterDeliveredIsRejected() != 0) {
        LogMerger_Cleanup();
        return 1;
    }
    return 0;
}

/*************************** End of file ****************************/
