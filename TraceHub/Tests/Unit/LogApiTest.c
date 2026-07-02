/*********************************************************************
*                             CineLogic                              *
*                      RTT Trace and Debug Bridge                    *
**********************************************************************
----------------------------------------------------------------------
File    : LogApiTest.c
Purpose : Unit checks for logging public APIs
---------------------------END-OF-HEADER------------------------------
*/

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "Log.h"
#include "TestCommon.h"

static int _TestTimestampedFileAndFormattedWrite(void) {
    FILE *file;
    FILE *blocker;
    char  buffer[128];
    char  long_prefix[5000];
    int   result;

    TEST_ASSERT(LOG_CreateTimestampedFileEx(NULL, "log", "w") == NULL);
    TEST_ASSERT(LOG_CreateTimestampedFileEx("tracehub_unit_log", NULL, "w") == NULL);
    TEST_ASSERT(LOG_CreateTimestampedFileEx("tracehub_unit_log", "", "w") == NULL);
    TEST_ASSERT(LOG_CreateTimestampedFileEx("tracehub_unit_log", "txt", NULL) == NULL);
    TEST_ASSERT(LOG_CreateTimestampedFileEx("tracehub_unit_log", "txt", "r") == NULL);
    TEST_ASSERT(LOG_CreateTimestampedFileEx("tracehub_unit_log", "txt", "rb") == NULL);
    TEST_ASSERT(LOG_CreateTimestampedFileEx("tracehub_unit_log", "txt", "x") == NULL);
    blocker = fopen("tracehub_unit_file_parent", "w");
    TEST_ASSERT(blocker != NULL);
    TEST_ASSERT(fclose(blocker) == 0);
    TEST_ASSERT(LOG_CreateTimestampedFileEx("tracehub_unit_file_parent/log", "txt", "w") == NULL);
    TEST_ASSERT(remove("tracehub_unit_file_parent") == 0);
    memset(long_prefix, 'p', sizeof(long_prefix) - 1u);
    long_prefix[sizeof(long_prefix) - 1u] = '\0';
    TEST_ASSERT(LOG_CreateTimestampedFileEx(long_prefix, "log", "w") == NULL);

    file = LOG_CreateTimestampedFileEx("tracehub_unit_log_api", "txt", "w+");
    TEST_ASSERT(file != NULL);
    LOG_LogToFile(file, "value %d\n", 7);
    result = Test_ReadTmpFile(file, buffer, sizeof(buffer));
    TEST_ASSERT(fclose(file) == 0);
    TEST_ASSERT(result == 0);
    TEST_ASSERT(strcmp(buffer, "value 7\n") == 0);

    file = LOG_CreateTimestampedFile("tracehub_unit_log_api_default");
    TEST_ASSERT(file != NULL);
    TEST_ASSERT(fclose(file) == 0);

    file = LOG_CreateTimestampedFileEx("tracehub_unit_log_api_append_text", "log", "a");
    TEST_ASSERT(file != NULL);
    TEST_ASSERT(fclose(file) == 0);

    file = LOG_CreateTimestampedFileEx("tracehub_unit_log_api_append", "bin", "ab");
    TEST_ASSERT(file != NULL);
    TEST_ASSERT(fclose(file) == 0);

    file = LOG_CreateTimestampedFileEx("tracehub_unit_log_api_append_plus_text", "log", "a+");
    TEST_ASSERT(file != NULL);
    TEST_ASSERT(fclose(file) == 0);

    file = LOG_CreateTimestampedFileEx("tracehub_unit_log_api_append_plus_binary", "bin", "a+b");
    TEST_ASSERT(file != NULL);
    TEST_ASSERT(fclose(file) == 0);

    file = LOG_CreateTimestampedFileEx("tracehub_unit_log_api_append_plus", "bin", "ab+");
    TEST_ASSERT(file != NULL);
    TEST_ASSERT(fclose(file) == 0);

    file = LOG_CreateTimestampedFileEx("tracehub_unit_log_api_write_text", "log", "w");
    TEST_ASSERT(file != NULL);
    TEST_ASSERT(fclose(file) == 0);

    file = LOG_CreateTimestampedFileEx("tracehub_unit_log_api_write_binary", "bin", "wb");
    TEST_ASSERT(file != NULL);
    TEST_ASSERT(fclose(file) == 0);

    file = LOG_CreateTimestampedFileEx("tracehub_unit_log_api_write_plus_binary", "bin", "w+b");
    TEST_ASSERT(file != NULL);
    TEST_ASSERT(fclose(file) == 0);

    file = LOG_CreateTimestampedFileEx("tracehub_unit_log_api_write_binary", "bin", "wb+");
    TEST_ASSERT(file != NULL);
    TEST_ASSERT(fclose(file) == 0);
    return 0;
}

static int _TestMainLogLifecycle(void) {
    FILE *main_file;

    LOG_Cleanup();
    TEST_ASSERT(LOG_GetMainFile() == NULL);
    TEST_ASSERT(LOG_InitEx(NULL) == -1);
    TEST_ASSERT(LOG_InitEx("tracehub_unit_main") == 0);
    main_file = LOG_GetMainFile();
    TEST_ASSERT(main_file != NULL);
    TEST_ASSERT(LOG_Init() == -1);

    LOG_Debug("unit", 1, "fn", "debug %d\n", 1);
    LOG_Info("info %d\n", 1);
    LOG_Warn("warn %d\n", 2);
    LOG_Error("error %d\n", 3);
    LOG_LogToFile(main_file, "main entry\n");

    LOG_Cleanup();
    TEST_ASSERT(LOG_GetMainFile() == NULL);
    TEST_ASSERT(LOG_Init() == 0);
    TEST_ASSERT(LOG_GetMainFile() != NULL);
    LOG_Cleanup();
    LOG_Cleanup();
    return 0;
}

static int _TestCleanTextWriters(void) {
    LOG_TextCleanState_t state;
    FILE                *file;
    char                 buffer[64];
    size_t               clean_len;
    int                  result;
    const char           c1_string[] = { 'C', (char)0x9d, 'x', (char)0x9c, 'D' };
    const char           invalid_utf8[] = { 'E', (char)0xc2, 'F', (char)0xf5, 'G' };
    const char           four_byte_utf8[] = {
        'H', (char)0xf0, (char)0x9f, (char)0x98, (char)0x80, 'I'
    };

    file = tmpfile();
    TEST_ASSERT(file != NULL);
    LOG_TextCleanStateInit(NULL);
    LOG_TextCleanStateInit(&state);
    TEST_ASSERT(LOG_WriteCleanTextToFile(file,
                                         "A\033[31mB\r\n",
                                         strlen("A\033[31mB\r\n"),
                                         &state) == 0);
    result = Test_ReadTmpFile(file, buffer, sizeof(buffer));
    TEST_ASSERT(fclose(file) == 0);
    TEST_ASSERT(result == 0);
    TEST_ASSERT(strcmp(buffer, "AB\n") == 0);

    file = tmpfile();
    TEST_ASSERT(file != NULL);
    clean_len = 123u;
    TEST_ASSERT(LOG_WriteCleanTextToFileEx(file, NULL, 1u, &state, &clean_len) == -1);
    TEST_ASSERT(clean_len == 0u);
    TEST_ASSERT(LOG_WriteCleanTextToFileEx(NULL, "x", 1u, &state, &clean_len) == 0);
    TEST_ASSERT(fclose(file) == 0);

    file = tmpfile();
    TEST_ASSERT(file != NULL);
    LOG_TextCleanStateInit(&state);
    TEST_ASSERT(LOG_WriteCleanTextToFileEx(file,
                                           "A\033]title",
                                           strlen("A\033]title"),
                                           &state,
                                           &clean_len) == 0);
    TEST_ASSERT(clean_len == 1u);
    TEST_ASSERT(LOG_WriteCleanTextToFileEx(file,
                                           "\033\\B",
                                           strlen("\033\\B"),
                                           &state,
                                           &clean_len) == 0);
    TEST_ASSERT(clean_len == 1u);
    TEST_ASSERT(LOG_WriteCleanTextToFileEx(file,
                                           c1_string,
                                           sizeof(c1_string),
                                           &state,
                                           &clean_len) == 0);
    TEST_ASSERT(clean_len == 2u);
    TEST_ASSERT(LOG_WriteCleanTextToFileEx(file,
                                           invalid_utf8,
                                           sizeof(invalid_utf8),
                                           &state,
                                           &clean_len) == 0);
    TEST_ASSERT(clean_len == 3u);
    TEST_ASSERT(LOG_WriteCleanTextToFileEx(file,
                                           four_byte_utf8,
                                           sizeof(four_byte_utf8),
                                           &state,
                                           &clean_len) == 0);
    TEST_ASSERT(clean_len == 6u);
    result = Test_ReadTmpFile(file, buffer, sizeof(buffer));
    TEST_ASSERT(fclose(file) == 0);
    TEST_ASSERT(result == 0);
    TEST_ASSERT(strcmp(buffer, "ABCDEFGH\xF0\x9F\x98\x80""I") == 0);
    return 0;
}

static int _TestCleanTextStateMachineBranches(void) {
    LOG_TextCleanState_t state;
    FILE                *file;
    char                 buffer[128];
    size_t               clean_len;
    int                  result;
    const char           esc_designator[] = { 'A', '\033', '(', '0', 'B' };
    const char           c1_csi[] = { 'C', (char)0x9b, '3', '1', 'm', 'D' };
    const char           c1_ss2[] = { 'E', (char)0x8e, 'x', 'F' };
    const char           utf8_c1_csi[] = {
        'G', (char)0xc2, (char)0x9b, '3', '1', 'm', 'H'
    };
    const char           utf8_three_byte[] = {
        'I', (char)0xe2, (char)0x82, (char)0xac, 'J'
    };
    const char           utf8_overlong[] = {
        'K', (char)0xe0, (char)0x80, (char)0x80, 'L'
    };
    const char           utf8_surrogate[] = {
        'M', (char)0xed, (char)0xa0, (char)0x80, 'N'
    };
    const char           utf8_above_max[] = {
        'O', (char)0xf4, (char)0x90, (char)0x80, (char)0x80, 'P'
    };
    const char           control_variants[] = {
        'Q',
        '\001',
        (char)0x84,
        '\033', 'X', 'R',
        '\033', ']', 'o', 's', 'c', '\007', 'S',
        '\033', 'P', 'd', (char)0x9c, 'T',
        (char)0xc2, 'A', 'U'
    };

    file = tmpfile();
    TEST_ASSERT(file != NULL);
    LOG_TextCleanStateInit(&state);
    TEST_ASSERT(LOG_WriteCleanTextToFileEx(file, "", 0u, &state, &clean_len) == 0);
    TEST_ASSERT(clean_len == 0u);
    TEST_ASSERT(LOG_WriteCleanTextToFileEx(file,
                                           esc_designator,
                                           sizeof(esc_designator),
                                           &state,
                                           &clean_len) == 0);
    TEST_ASSERT(clean_len == 2u);
    TEST_ASSERT(LOG_WriteCleanTextToFileEx(file,
                                           c1_csi,
                                           sizeof(c1_csi),
                                           &state,
                                           &clean_len) == 0);
    TEST_ASSERT(clean_len == 2u);
    TEST_ASSERT(LOG_WriteCleanTextToFileEx(file,
                                           c1_ss2,
                                           sizeof(c1_ss2),
                                           &state,
                                           &clean_len) == 0);
    TEST_ASSERT(clean_len == 2u);
    TEST_ASSERT(LOG_WriteCleanTextToFileEx(file,
                                           utf8_c1_csi,
                                           sizeof(utf8_c1_csi),
                                           &state,
                                           &clean_len) == 0);
    TEST_ASSERT(clean_len == 2u);
    TEST_ASSERT(LOG_WriteCleanTextToFileEx(file,
                                           utf8_three_byte,
                                           sizeof(utf8_three_byte),
                                           &state,
                                           &clean_len) == 0);
    TEST_ASSERT(clean_len == sizeof(utf8_three_byte));
    TEST_ASSERT(LOG_WriteCleanTextToFileEx(file,
                                           utf8_overlong,
                                           sizeof(utf8_overlong),
                                           &state,
                                           &clean_len) == 0);
    TEST_ASSERT(clean_len == 2u);
    TEST_ASSERT(LOG_WriteCleanTextToFileEx(file,
                                           utf8_surrogate,
                                           sizeof(utf8_surrogate),
                                           &state,
                                           &clean_len) == 0);
    TEST_ASSERT(clean_len == 2u);
    TEST_ASSERT(LOG_WriteCleanTextToFileEx(file,
                                           utf8_above_max,
                                           sizeof(utf8_above_max),
                                           &state,
                                           &clean_len) == 0);
    TEST_ASSERT(clean_len == 2u);
    TEST_ASSERT(LOG_WriteCleanTextToFileEx(file,
                                           control_variants,
                                           sizeof(control_variants),
                                           &state,
                                           &clean_len) == 0);
    TEST_ASSERT(clean_len == 6u);
    result = Test_ReadTmpFile(file, buffer, sizeof(buffer));
    TEST_ASSERT(fclose(file) == 0);
    TEST_ASSERT(result == 0);
    TEST_ASSERT(strcmp(buffer, "ABCDEFGHI\xE2\x82\xAC""JKLMNOPQRSTAU") == 0);

    file = tmpfile();
    TEST_ASSERT(file != NULL);
    LOG_TextCleanStateInit(&state);
    state.utf8_expected = 1u;
    state.utf8_length = (unsigned)sizeof(state.utf8_bytes);
    clean_len = 123u;
    TEST_ASSERT(LOG_WriteCleanTextToFileEx(file, "\x80", 1u, &state, &clean_len) == 0);
    TEST_ASSERT(clean_len == 0u);
    TEST_ASSERT(state.utf8_expected == 0u);
    TEST_ASSERT(state.utf8_length == 0u);
    TEST_ASSERT(fclose(file) == 0);

    file = tmpfile();
    TEST_ASSERT(file != NULL);
    LOG_TextCleanStateInit(&state);
    state.state = 999u;
    clean_len = 123u;
    TEST_ASSERT(LOG_WriteCleanTextToFileEx(file, "x", 1u, &state, &clean_len) == -1);
    TEST_ASSERT(clean_len == 0u);
    TEST_ASSERT(LOG_WriteCleanTextToFileEx(file, "x", 1u, NULL, &clean_len) == -1);
    TEST_ASSERT(clean_len == 0u);
    TEST_ASSERT(fclose(file) == 0);
    return 0;
}

static int _TestSwimLaneLogWriterContracts(void) {
    LOG_TextCleanState_t state;
    FILE                *file;
    FILE                *error_file;
    char                 buffer[128];
    int                  result;

    LOG_TextCleanStateInit(&state);
    TEST_ASSERT(LOG_SwimLaneLogToFile(NULL, 1u, "LINUX", "content", &state) == 0);

    file = tmpfile();
    TEST_ASSERT(file != NULL);
    TEST_ASSERT(LOG_SwimLaneLogToFile(file, 1u, NULL, "content", &state) == -1);
    TEST_ASSERT(LOG_SwimLaneLogToFile(file, 1u, "LINUX", NULL, &state) == -1);
    TEST_ASSERT(LOG_SwimLaneLogToFile(file, 1u, "LINUX", "content", NULL) == -1);

    error_file = tmpfile();
    TEST_ASSERT(error_file != NULL);
    LOG_TextCleanStateInit(&state);
    state.state = 999u;
    TEST_ASSERT(LOG_SwimLaneLogToFile(error_file, 1u, "LINUX", "bad", &state) == -1);
    TEST_ASSERT(fclose(error_file) == 0);

    LOG_TextCleanStateInit(&state);
    TEST_ASSERT(LOG_SwimLaneLogToFile(file,
                                      123u,
                                      "RTOS",
                                      "\033[32mready\033[0m",
                                      &state) == 0);
    result = Test_ReadTmpFile(file, buffer, sizeof(buffer));
    TEST_ASSERT(fclose(file) == 0);
    TEST_ASSERT(result == 0);
    TEST_ASSERT(strcmp(buffer, "[123] [RTOS] ready\n") == 0);
    return 0;
}

static int _TestHexdumpApiIsCallable(void) {
    unsigned char data[2];
    unsigned char wide_data[17];
    unsigned      i;

    data[0] = 0x12u;
    data[1] = 0x34u;
    for (i = 0u; i < sizeof(wide_data); i++) {
        wide_data[i] = (unsigned char)i;
    }
    LOG_Hexdump(data, 0u, true, true);
    LOG_Hexdump(data, sizeof(data), true, false);
    LOG_Hexdump(data, sizeof(data), false, true);
    LOG_Hexdump(wide_data, sizeof(wide_data), false, false);
    return 0;
}

int main(void) {
    TEST_RUN(_TestTimestampedFileAndFormattedWrite);
    TEST_RUN(_TestMainLogLifecycle);
    TEST_RUN(_TestCleanTextWriters);
    TEST_RUN(_TestCleanTextStateMachineBranches);
    TEST_RUN(_TestSwimLaneLogWriterContracts);
    TEST_RUN(_TestHexdumpApiIsCallable);
    return 0;
}

/*************************** End of file ****************************/
