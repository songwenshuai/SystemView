/*********************************************************************
*                             CineLogic                              *
*                      RTT Trace and Debug Bridge                    *
**********************************************************************
----------------------------------------------------------------------
File    : TestCommon.h
Purpose : Shared unit test assertions and helpers
---------------------------END-OF-HEADER------------------------------
*/

#ifndef TRACEHUB_TEST_COMMON_H
#define TRACEHUB_TEST_COMMON_H

#include <stdio.h>

#define TEST_ASSERT(expr)                                                   \
    do {                                                                    \
        if (!(expr)) {                                                       \
            fprintf(stderr, "%s:%d: assertion failed: %s\n",                \
                    __FILE__, __LINE__, #expr);                             \
            return -1;                                                      \
        }                                                                   \
    } while (0)

#define TEST_RUN(fn)                                                        \
    do {                                                                    \
        if ((fn)() != 0) {                                                   \
            return 1;                                                       \
        }                                                                   \
    } while (0)

static int Test_ReadTmpFile(FILE *file, char *buffer, size_t buffer_size) {
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

#endif

/*************************** End of file ****************************/
