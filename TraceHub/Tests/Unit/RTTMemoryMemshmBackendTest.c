/*********************************************************************
*                             CineLogic                              *
*                      RTT Trace and Debug Bridge                    *
**********************************************************************
----------------------------------------------------------------------
File    : RTTMemoryMemshmBackendTest.c
Purpose : Unit checks for the real MEMSHM RTT memory backend
---------------------------END-OF-HEADER------------------------------
*/

#include <stdint.h>
#include <string.h>

#include "RTTMemory.h"
#include "TestCommon.h"

#define TEST_MEMSHM_NAME  "/tracehub_unit_memshm_backend"
#define TEST_MEMSHM_BASE  0x40000000u
#define TEST_MEMSHM_SIZE  4096u

static int _TestMemshmBackendMapping(void) {
    const RTTMem_Backend_t *backend;
    uintptr_t              local;
    unsigned char         *bytes;

    RTTMem_Cleanup();
    backend = RTTMem_Backend_MEMSHM();
    TEST_ASSERT(backend != NULL);
    TEST_ASSERT(strcmp(backend->name, "memshm") == 0);
    TEST_ASSERT(RTTMem_InstallBackend(backend) == 0);
    TEST_ASSERT(strcmp(RTTMem_GetBackendName(), "memshm") == 0);

    RTTMem_SetResetOnInit(true);
    TEST_ASSERT(RTTMem_InitEx(TEST_MEMSHM_NAME,
                              TEST_MEMSHM_BASE,
                              TEST_MEMSHM_SIZE) == 0);
    TEST_ASSERT(RTTMem_GetBackendBase() == TEST_MEMSHM_BASE);
    TEST_ASSERT(RTTMem_GetMappedSize() == TEST_MEMSHM_SIZE);

    local = RTTMem_ToLocalAddress(TEST_MEMSHM_BASE + 16u, 4u);
    TEST_ASSERT(local != 0u);
    bytes = (unsigned char *)local;
    bytes[0] = 0x12u;
    bytes[1] = 0x34u;
    bytes[2] = 0x56u;
    bytes[3] = 0x78u;
    TEST_ASSERT(bytes[0] == 0x12u);
    TEST_ASSERT(bytes[1] == 0x34u);
    TEST_ASSERT(bytes[2] == 0x56u);
    TEST_ASSERT(bytes[3] == 0x78u);

    TEST_ASSERT(RTTMem_ToLocalAddress(TEST_MEMSHM_BASE - 1u, 1u) == 0u);
    TEST_ASSERT(RTTMem_ToLocalAddress(TEST_MEMSHM_BASE + TEST_MEMSHM_SIZE, 1u) == 0u);
    TEST_ASSERT(RTTMem_ToLocalAddress(TEST_MEMSHM_BASE + TEST_MEMSHM_SIZE - 1u, 2u) == 0u);

    RTTMem_Cleanup();
    TEST_ASSERT(RTTMem_GetBackendBase() == 0u);
    TEST_ASSERT(RTTMem_GetMappedSize() == 0u);
    RTTMem_SetResetOnInit(false);
    return 0;
}

int main(void) {
    TEST_RUN(_TestMemshmBackendMapping);
    return 0;
}

/*************************** End of file ****************************/
