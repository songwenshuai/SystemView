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

int Test_ReadTmpFile(FILE *file, char *buffer, size_t buffer_size);

#endif

/*************************** End of file ****************************/
