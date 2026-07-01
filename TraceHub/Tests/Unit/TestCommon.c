/*********************************************************************
*                             CineLogic                              *
*                      RTT Trace and Debug Bridge                    *
**********************************************************************
----------------------------------------------------------------------
File    : TestCommon.c
Purpose : Shared unit test helpers
---------------------------END-OF-HEADER------------------------------
*/

#include "TestCommon.h"

int Test_ReadTmpFile(FILE *file, char *buffer, size_t buffer_size) {
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

/*************************** End of file ****************************/
