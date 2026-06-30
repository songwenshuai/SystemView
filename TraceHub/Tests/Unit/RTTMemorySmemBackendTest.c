/*********************************************************************
*                             CineLogic                              *
*                      RTT Trace and Debug Bridge                    *
**********************************************************************
----------------------------------------------------------------------
File    : RTTMemorySmemBackendTest.c
Purpose : Unit checks for the Linux SMEM RTT memory backend factory
---------------------------END-OF-HEADER------------------------------
*/

#include <stdarg.h>
#include <string.h>

#include "RTTMemory.h"
#include "TestCommon.h"

void LOG_Debug(const char *file, int line, const char *function, const char *sFormat, ...) {
    (void)file;
    (void)line;
    (void)function;
    (void)sFormat;
}

void LOG_Error(const char *sFormat, ...) {
    (void)sFormat;
}

void LOG_Warn(const char *sFormat, ...) {
    (void)sFormat;
}

static int _TestSmemBackendFactoryAndMissingDevice(void) {
    const RTTMem_Backend_t *backend;

    RTTMem_Cleanup();
    backend = RTTMem_Backend_SMEM();
    TEST_ASSERT(backend != NULL);
    TEST_ASSERT(strcmp(backend->name, "smem") == 0);
    TEST_ASSERT(RTTMem_InstallBackend(backend) == 0);
    TEST_ASSERT(strcmp(RTTMem_GetBackendName(), "smem") == 0);
    TEST_ASSERT(RTTMem_InitEx("/tracehub_unit_missing_smem_device", 0u, 0u) == -1);
    TEST_ASSERT(RTTMem_GetBackendBase() == 0u);
    TEST_ASSERT(RTTMem_GetMappedSize() == 0u);
    TEST_ASSERT(RTTMem_ToLocalAddress(1u, 1u) == 0u);
    RTTMem_Cleanup();
    return 0;
}

int main(void) {
    TEST_RUN(_TestSmemBackendFactoryAndMissingDevice);
    return 0;
}

/*************************** End of file ****************************/
