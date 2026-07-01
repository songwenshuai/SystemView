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
    char  buffer[128];
    int   result;

    TEST_ASSERT(LOG_CreateTimestampedFileEx(NULL, "log", "w") == NULL);
    TEST_ASSERT(LOG_CreateTimestampedFileEx("tracehub_unit_log", "", "w") == NULL);
    TEST_ASSERT(LOG_CreateTimestampedFileEx("tracehub_unit_log", "txt", "r") == NULL);

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
    return 0;
}

static int _TestSwimLaneLogWriterContracts(void) {
    LOG_TextCleanState_t state;
    FILE                *file;
    char                 buffer[128];
    int                  result;

    LOG_TextCleanStateInit(&state);
    TEST_ASSERT(LOG_SwimLaneLogToFile(NULL, 1u, "LINUX", "content", &state) == 0);

    file = tmpfile();
    TEST_ASSERT(file != NULL);
    TEST_ASSERT(LOG_SwimLaneLogToFile(file, 1u, NULL, "content", &state) == -1);
    TEST_ASSERT(LOG_SwimLaneLogToFile(file, 1u, "LINUX", NULL, &state) == -1);
    TEST_ASSERT(LOG_SwimLaneLogToFile(file, 1u, "LINUX", "content", NULL) == -1);

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

    data[0] = 0x12u;
    data[1] = 0x34u;
    LOG_Hexdump(data, 0u, true, true);
    LOG_Hexdump(data, sizeof(data), true, false);
    return 0;
}

int main(void) {
    TEST_RUN(_TestTimestampedFileAndFormattedWrite);
    TEST_RUN(_TestMainLogLifecycle);
    TEST_RUN(_TestCleanTextWriters);
    TEST_RUN(_TestSwimLaneLogWriterContracts);
    TEST_RUN(_TestHexdumpApiIsCallable);
    return 0;
}

/*************************** End of file ****************************/
