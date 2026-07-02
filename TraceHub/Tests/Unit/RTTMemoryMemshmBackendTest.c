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

#if !defined(_WIN32)
  #include <fcntl.h>
  #include <sys/mman.h>
  #include <sys/stat.h>
  #include <unistd.h>
#endif

#include "RTTMemory.h"
#include "TestCommon.h"

#define TEST_MEMSHM_NAME          "/tracehub_unit_memshm_backend"
#define TEST_MEMSHM_DEFAULT_NAME  "/th_memshm_def"
#define TEST_MEMSHM_SMALL_NAME    "/th_memshm_small"
#define TEST_MEMSHM_BASE          0x40000000u
#define TEST_MEMSHM_SIZE          4096u

static int _TestMemshmBackendRejectsInvalidNames(void) {
    const RTTMem_Backend_t *backend;
    char                    long_name[300];

    memset(long_name, 'a', sizeof(long_name));
    long_name[0] = '/';
    long_name[sizeof(long_name) - 1u] = '\0';

    RTTMem_Cleanup();
    backend = RTTMem_Backend_MEMSHM();
    TEST_ASSERT(backend != NULL);
    TEST_ASSERT(RTTMem_InstallBackend(backend) == 0);
#if !defined(_WIN32)
    TEST_ASSERT(RTTMem_InitEx("relative_name", TEST_MEMSHM_BASE, TEST_MEMSHM_SIZE) == -1);
#endif
    TEST_ASSERT(RTTMem_InitEx(long_name, TEST_MEMSHM_BASE, TEST_MEMSHM_SIZE) == -1);
    RTTMem_Cleanup();
    return 0;
}

static int _TestMemshmBackendMapping(void) {
    const RTTMem_Backend_t *backend;
    uintptr_t              local;
    size_t                 mapped_size;
    unsigned char         *bytes;

    RTTMem_Cleanup();
    backend = RTTMem_Backend_MEMSHM();
    TEST_ASSERT(backend != NULL);
    TEST_ASSERT(strcmp(backend->name, "memshm") == 0);
    TEST_ASSERT(RTTMem_InstallBackend(backend) == 0);
    TEST_ASSERT(strcmp(RTTMem_GetBackendName(), "memshm") == 0);

    TEST_ASSERT(RTTMem_InitEx(NULL, TEST_MEMSHM_BASE, TEST_MEMSHM_SIZE) == -1);
    TEST_ASSERT(RTTMem_InitEx("", TEST_MEMSHM_BASE, TEST_MEMSHM_SIZE) == -1);

    RTTMem_SetResetOnInit(true);
    TEST_ASSERT(RTTMem_InitEx(TEST_MEMSHM_NAME,
                              TEST_MEMSHM_BASE,
                              TEST_MEMSHM_SIZE) == 0);
    TEST_ASSERT(RTTMem_GetBackendBase() == TEST_MEMSHM_BASE);
    mapped_size = RTTMem_GetMappedSize();
    TEST_ASSERT(mapped_size >= TEST_MEMSHM_SIZE);

    TEST_ASSERT(RTTMem_InitEx(TEST_MEMSHM_NAME,
                              TEST_MEMSHM_BASE,
                              TEST_MEMSHM_SIZE) == 0);
    TEST_ASSERT(RTTMem_GetBackendBase() == TEST_MEMSHM_BASE);
    TEST_ASSERT(RTTMem_GetMappedSize() == mapped_size);

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
    TEST_ASSERT(RTTMem_ToLocalAddress(TEST_MEMSHM_BASE + mapped_size, 1u) == 0u);
    TEST_ASSERT(RTTMem_ToLocalAddress(TEST_MEMSHM_BASE + mapped_size - 1u, 2u) == 0u);

    RTTMem_Cleanup();
    TEST_ASSERT(RTTMem_GetBackendBase() == 0u);
    TEST_ASSERT(RTTMem_GetMappedSize() == 0u);
    RTTMem_SetResetOnInit(false);
    return 0;
}

static int _TestMemshmBackendRejectsTooSmallExistingObject(void) {
#if !defined(_WIN32)
    const RTTMem_Backend_t *backend;
    struct stat             st;
    size_t                  requested_size;
    int                     fd;

    RTTMem_Cleanup();
    backend = RTTMem_Backend_MEMSHM();
    TEST_ASSERT(backend != NULL);
    TEST_ASSERT(RTTMem_InstallBackend(backend) == 0);

    (void)shm_unlink(TEST_MEMSHM_SMALL_NAME);
    fd = shm_open(TEST_MEMSHM_SMALL_NAME, O_RDWR | O_CREAT | O_EXCL, 0666);
    TEST_ASSERT(fd >= 0);
    TEST_ASSERT(ftruncate(fd, 64) == 0);
    TEST_ASSERT(fstat(fd, &st) == 0);
    TEST_ASSERT(st.st_size >= 0);
    requested_size = (size_t)st.st_size + 1u;
    TEST_ASSERT(requested_size > (size_t)st.st_size);
    TEST_ASSERT(close(fd) == 0);

    RTTMem_SetResetOnInit(false);
    TEST_ASSERT(RTTMem_InitEx(TEST_MEMSHM_SMALL_NAME,
                              TEST_MEMSHM_BASE,
                              requested_size) == -1);
    TEST_ASSERT(RTTMem_GetBackendBase() == 0u);
    TEST_ASSERT(RTTMem_GetMappedSize() == 0u);

    (void)shm_unlink(TEST_MEMSHM_SMALL_NAME);
    RTTMem_Cleanup();
    RTTMem_SetResetOnInit(false);
#endif
    return 0;
}

static int _TestMemshmBackendDefaultsAndExistingMapping(void) {
    const RTTMem_Backend_t *backend;
    uintptr_t              local;
    size_t                 first_size;
    unsigned char         *bytes;

    RTTMem_Cleanup();
    backend = RTTMem_Backend_MEMSHM();
    TEST_ASSERT(backend != NULL);
    TEST_ASSERT(RTTMem_InstallBackend(backend) == 0);

    RTTMem_SetResetOnInit(true);
    TEST_ASSERT(RTTMem_InitEx(TEST_MEMSHM_DEFAULT_NAME, 0u, 0u) == 0);
    TEST_ASSERT(RTTMem_GetBackendBase() != 0u);
    first_size = RTTMem_GetMappedSize();
    TEST_ASSERT(first_size > 0u);

    local = RTTMem_ToLocalAddress(RTTMem_GetBackendBase(), 1u);
    TEST_ASSERT(local != 0u);
    bytes = (unsigned char *)local;
    bytes[0] = 0x5au;
    TEST_ASSERT(bytes[0] == 0x5au);

    RTTMem_Cleanup();
    TEST_ASSERT(RTTMem_GetBackendBase() == 0u);
    TEST_ASSERT(RTTMem_GetMappedSize() == 0u);

    RTTMem_SetResetOnInit(false);
    TEST_ASSERT(RTTMem_InitEx(TEST_MEMSHM_DEFAULT_NAME, 0u, 0u) == 0);
    TEST_ASSERT(RTTMem_GetBackendBase() != 0u);
    TEST_ASSERT(RTTMem_GetMappedSize() >= first_size);

    RTTMem_Cleanup();
    TEST_ASSERT(RTTMem_ToLocalAddress(RTTMem_GetBackendBase(), 1u) == 0u);
    RTTMem_SetResetOnInit(false);
    return 0;
}

int main(void) {
    TEST_RUN(_TestMemshmBackendRejectsInvalidNames);
    TEST_RUN(_TestMemshmBackendMapping);
    TEST_RUN(_TestMemshmBackendRejectsTooSmallExistingObject);
    TEST_RUN(_TestMemshmBackendDefaultsAndExistingMapping);
    return 0;
}

/*************************** End of file ****************************/
