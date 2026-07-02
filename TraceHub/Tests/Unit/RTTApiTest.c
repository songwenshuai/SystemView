/*********************************************************************
*                             CineLogic                              *
*                      RTT Trace and Debug Bridge                    *
**********************************************************************
----------------------------------------------------------------------
File    : RTTApiTest.c
Purpose : Unit checks for RTT memory and bridge public APIs
---------------------------END-OF-HEADER------------------------------
*/

#include <stdbool.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "RTTMemory.h"
#include "RTTBridge.h"
#include "RTTBridge_internal.h"
#include "SEGGER_RTT.h"
#include "TestCommon.h"

static uint64_t  _stub_base;
static size_t    _stub_size;
static uintptr_t _stub_local_base;
static int       _stub_init_result;
static unsigned  _stub_init_count;
static unsigned  _stub_cleanup_count;
static const char *_stub_path;
static uint64_t  _stub_init_base;
static size_t    _stub_init_size;

static int       _stub_find_result;
static size_t    _stub_find_region_size;
static unsigned  _stub_find_failures_before_success;
static unsigned  _stub_find_call_count;
static int       _stub_check_init_result;
static int       _stub_check_region_result;
static int       _stub_check_up_result;
static int       _stub_check_down_result;
static unsigned  _stub_bytes_in_buffer;
static unsigned  _stub_read_result;
static unsigned  _stub_write_result;
static bool      _stub_write_override_enabled;
static int       _stub_ensure_result;
static unsigned  _stub_sleep_count;
static bool      _stub_force_map_base_failure;
static bool      _stub_force_search_base_failure;

static void _ResetStubs(void) {
    _stub_base = 0x10000000u;
    _stub_size = 0x1000u;
    _stub_local_base = 0x20000000u;
    _stub_init_result = 0;
    _stub_init_count = 0u;
    _stub_cleanup_count = 0u;
    _stub_path = NULL;
    _stub_init_base = 0u;
    _stub_init_size = 0u;
    _stub_find_result = 0;
    _stub_find_region_size = 0x1000u;
    _stub_find_failures_before_success = 0u;
    _stub_find_call_count = 0u;
    _stub_check_init_result = 0;
    _stub_check_region_result = 0;
    _stub_check_up_result = 0;
    _stub_check_down_result = 0;
    _stub_bytes_in_buffer = 3u;
    _stub_read_result = 2u;
    _stub_write_result = 0u;
    _stub_write_override_enabled = false;
    _stub_ensure_result = 0;
    _stub_sleep_count = 0u;
    _stub_force_map_base_failure = false;
    _stub_force_search_base_failure = false;
}

static void _ResetRegionSearchState(void) {
    _ResetStubs();
    memset(&_bridge_state, 0, sizeof(_bridge_state));
}

static int _BackendInit(const char *path, uint64_t base, size_t size) {
    _stub_path = path;
    _stub_init_base = base;
    _stub_init_size = size;
    _stub_init_count++;
    return _stub_init_result;
}

static void _BackendCleanup(void) {
    _stub_cleanup_count++;
}

static uint64_t _BackendGetBase(void) {
    return _stub_base;
}

static size_t _BackendGetSize(void) {
    return _stub_size;
}

static uintptr_t _BackendToLocal(uint64_t address, size_t size) {
    uint64_t offset;

    if (_stub_force_map_base_failure && address == _stub_base && size == _stub_size) {
        return 0u;
    }
    if (_stub_force_search_base_failure && address != _stub_base) {
        return 0u;
    }
    if (address < _stub_base) {
        return 0u;
    }
    offset = address - _stub_base;
    if (offset > (uint64_t)_stub_size || size > (_stub_size - (size_t)offset)) {
        return 0u;
    }
    return _stub_local_base + (uintptr_t)offset;
}

static const RTTMem_Backend_t _stub_backend = {
    "unit",
    _BackendInit,
    _BackendCleanup,
    _BackendGetBase,
    _BackendGetSize,
    _BackendToLocal
};

const RTTMem_Backend_t *RTTMem_Backend_MEMSHM(void) {
    return &_stub_backend;
}

int SEGGER_RTT_EnsureInitEx(PTR_ADDR Address, size_t Size, unsigned NumBuffers) {
    (void)Address;
    (void)Size;
    (void)NumBuffers;
    return _stub_ensure_result;
}

int SEGGER_RTT_CheckInit(PTR_ADDR Address) {
    (void)Address;
    return _stub_check_init_result;
}

int SEGGER_RTT_CheckRegion(PTR_ADDR Address, size_t Size) {
    (void)Address;
    (void)Size;
    return _stub_check_region_result;
}

int SEGGER_RTT_CheckUpBuffer(PTR_ADDR Address, size_t Size, unsigned BufferIndex) {
    (void)Address;
    (void)Size;
    (void)BufferIndex;
    return _stub_check_up_result;
}

int SEGGER_RTT_CheckDownBuffer(PTR_ADDR Address, size_t Size, unsigned BufferIndex) {
    (void)Address;
    (void)Size;
    (void)BufferIndex;
    return _stub_check_down_result;
}

int SEGGER_RTT_FindValidControlBlock(PTR_ADDR *pAddress, size_t Size, size_t *pRegionSize) {
    _stub_find_call_count++;
    if (_stub_find_result != 0) {
        return _stub_find_result;
    }
    if (_stub_find_failures_before_success > 0u) {
        _stub_find_failures_before_success--;
        return -1;
    }
    if (pAddress == NULL || pRegionSize == NULL) {
        return -1;
    }
    *pRegionSize = (_stub_find_region_size == 0u) ? Size : _stub_find_region_size;
    return 0;
}

unsigned SEGGER_RTT_GetBytesInBuffer(PTR_ADDR Address, unsigned BufferIndex) {
    (void)Address;
    (void)BufferIndex;
    return _stub_bytes_in_buffer;
}

unsigned SEGGER_RTT_ReadUpBufferNoLock(PTR_ADDR Address,
                                       unsigned BufferIndex,
                                       void *pData,
                                       unsigned BufferSize) {
    (void)Address;
    (void)BufferIndex;
    if (pData != NULL && BufferSize >= 2u && _stub_read_result >= 2u) {
        memcpy(pData, "rt", 2u);
    }
    return _stub_read_result;
}

unsigned SEGGER_RTT_WriteDownBufferNoLock(PTR_ADDR Address,
                                          unsigned BufferIndex,
                                          const void *pBuffer,
                                          unsigned NumBytes) {
    (void)Address;
    (void)BufferIndex;
    (void)pBuffer;
    if (_stub_write_override_enabled) {
        return _stub_write_result;
    }
    return NumBytes;
}

int SYS_MutexLock(SYS_Mutex *mutex) {
    (void)mutex;
    return 0;
}

int SYS_MutexUnlock(SYS_Mutex *mutex) {
    (void)mutex;
    return 0;
}

void SYS_Sleep(unsigned ms) {
    (void)ms;
    _stub_sleep_count++;
}

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

static int _TestMemoryDispatcher(void) {
    RTTMem_Backend_t invalid_backend;

    _ResetStubs();
    RTTMem_Cleanup();
    TEST_ASSERT(RTTMem_GetBackendName() != NULL);
    TEST_ASSERT(RTTMem_InstallBackend(NULL) == -1);

    memset(&invalid_backend, 0, sizeof(invalid_backend));
    TEST_ASSERT(RTTMem_InstallBackend(&invalid_backend) == -1);

    TEST_ASSERT(RTTMem_InstallBackend(&_stub_backend) == 0);
    TEST_ASSERT(RTTMem_GetBackend() == &_stub_backend);
    TEST_ASSERT(strcmp(RTTMem_GetBackendName(), "unit") == 0);

    RTTMem_SetResetOnInit(true);
    TEST_ASSERT(RTTMem_GetResetOnInit());
    RTTMem_SetResetOnInit(false);
    TEST_ASSERT(!RTTMem_GetResetOnInit());

    TEST_ASSERT(RTTMem_InitEx("path", 0x111u, 0x222u) == 0);
    TEST_ASSERT(_stub_init_count == 1u);
    TEST_ASSERT(strcmp(_stub_path, "path") == 0);
    TEST_ASSERT(_stub_init_base == 0x111u);
    TEST_ASSERT(_stub_init_size == 0x222u);
    TEST_ASSERT(RTTMem_GetBackendBase() == _stub_base);
    TEST_ASSERT(RTTMem_GetMappedSize() == _stub_size);
    TEST_ASSERT(RTTMem_ToLocalAddress(_stub_base + 8u, 4u) == _stub_local_base + 8u);
    TEST_ASSERT(RTTMem_ToLocalAddress(_stub_base + _stub_size, 1u) == 0u);
    RTTMem_Cleanup();
    TEST_ASSERT(_stub_cleanup_count == 1u);

    TEST_ASSERT(RTTMem_InstallDefaultBackend() == 0);
    TEST_ASSERT(strcmp(RTTMem_GetBackendName(), "unit") == 0);
    TEST_ASSERT(RTTMem_Init("default") == 0);
    TEST_ASSERT(_stub_init_count == 2u);
    return 0;
}

static int _TestBridgeInvalidStateContracts(void) {
    RTTBridge_Config_t config;
    char               buffer[4];

    _ResetStubs();
    RTTBridge_Cleanup();
    TEST_ASSERT(RTTBridge_Init(NULL) == -1);

    memset(&config, 0, sizeof(config));
    config.device_path = "stub";
    config.rtt_region_size = 1u;
    TEST_ASSERT(RTTBridge_Init(&config) == -1);

    TEST_ASSERT(RTTBridge_GetState() != NULL);
    TEST_ASSERT(!RTTBridge_IsRunning());
    TEST_ASSERT(RTTBridge_GetValidatedRTTRegion(NULL, NULL) == -1);
    TEST_ASSERT(RTTBridge_CheckUpBufferChannel(0u) == -1);
    TEST_ASSERT(RTTBridge_CheckDownBufferChannel(0u) == -1);
    TEST_ASSERT(RTTBridge_GetBytesInBuffer(0u) == -1);
    TEST_ASSERT(RTTBridge_ReadUpBufferNoLock(0u, NULL, sizeof(buffer)) == -1);
    TEST_ASSERT(RTTBridge_WriteDownBufferNoLock(0u, NULL, sizeof(buffer)) == -1);
    TEST_ASSERT(RTTBridge_WaitForRTTInitialized(0u, 1u, 1u) == -1);
    TEST_ASSERT(RTTBridge_WaitForRTTInitialized(1u, 1u, 0u) == -1);
    TEST_ASSERT(RTTBridge_WaitForRTTUpChannelReady(0u, 1u, 0u, 1u, 1u) == -1);
    TEST_ASSERT(RTTBridge_WaitForRTTUpChannelReady(1u, 0u, 0u, 1u, 1u) == -1);
    TEST_ASSERT(RTTBridge_EnsureRTTInitialized(1u, 2u, 3u) == 0);
    RTTBridge_SetRunning(true);
    RTTBridge_IncrementPolls();
    RTTBridge_IncrementErrors();
    TEST_ASSERT(RTTBridge_GetState()->polls_count == 0u);
    TEST_ASSERT(RTTBridge_GetState()->errors_count == 0u);
    RTTBridge_Status();
    return 0;
}

static int _TestBridgeInitAbortCleansBackend(void) {
    RTTBridge_Config_t config;
    volatile sig_atomic_t run_flag;

    _ResetStubs();
    RTTBridge_Cleanup();
    run_flag = 0;
    memset(&config, 0, sizeof(config));
    config.device_path = "stub";
    config.run_flag = &run_flag;

    TEST_ASSERT(RTTBridge_Init(&config) == -3);
    TEST_ASSERT(_stub_init_count == 1u);
    TEST_ASSERT(_stub_cleanup_count == 1u);
    TEST_ASSERT(!RTTBridge_GetState()->initialized);
    return 0;
}

static int _TestBridgeInitFailureAndRetryPaths(void) {
    RTTBridge_Config_t config;
    volatile sig_atomic_t run_flag;

    run_flag = 1;

    _ResetStubs();
    RTTBridge_Cleanup();
    memset(&config, 0, sizeof(config));
    config.device_path = "stub";
    config.run_flag = &run_flag;
    _stub_init_result = -1;

    TEST_ASSERT(RTTBridge_Init(&config) == -1);
    TEST_ASSERT(_stub_init_count == 1u);
    TEST_ASSERT(_stub_cleanup_count == 0u);
    TEST_ASSERT(!RTTBridge_GetState()->initialized);

    _ResetStubs();
    RTTBridge_Cleanup();
    memset(&config, 0, sizeof(config));
    config.device_path = "stub";
    config.run_flag = &run_flag;
    _stub_size = 0u;

    TEST_ASSERT(RTTBridge_Init(&config) == -1);
    TEST_ASSERT(_stub_find_call_count == 0u);
    TEST_ASSERT(_stub_cleanup_count == 1u);
    TEST_ASSERT(!RTTBridge_GetState()->initialized);

    _ResetStubs();
    RTTBridge_Cleanup();
    memset(&config, 0, sizeof(config));
    config.device_path = "stub";
    config.run_flag = &run_flag;
    config.rtt_search_timeout_ms = 1u;
    _stub_find_result = -1;

    TEST_ASSERT(RTTBridge_Init(&config) == -2);
    TEST_ASSERT(_stub_find_call_count == 2u);
    TEST_ASSERT(_stub_sleep_count == 1u);
    TEST_ASSERT(_stub_cleanup_count == 1u);
    TEST_ASSERT(!RTTBridge_GetState()->initialized);

    _ResetStubs();
    RTTBridge_Cleanup();
    memset(&config, 0, sizeof(config));
    config.device_path = "stub";
    config.run_flag = &run_flag;
    config.rtt_search_timeout_ms = 2000u;
    _stub_find_failures_before_success = 1u;

    TEST_ASSERT(RTTBridge_Init(&config) == 0);
    TEST_ASSERT(_stub_find_call_count == 2u);
    TEST_ASSERT(_stub_sleep_count == 1u);
    TEST_ASSERT(RTTBridge_GetState()->initialized);
    RTTBridge_Cleanup();
    return 0;
}

static int _TestBridgeInitializedOperations(void) {
    RTTBridge_Config_t config;
    volatile sig_atomic_t run_flag;
    uintptr_t address;
    size_t    region_size;
    char      buffer[8];

    _ResetStubs();
    RTTBridge_Cleanup();
    run_flag = 1;
    memset(&config, 0, sizeof(config));
    config.device_path = "stub";
    config.run_flag = &run_flag;

    TEST_ASSERT(RTTBridge_Init(&config) == 0);
    TEST_ASSERT(RTTBridge_GetState()->initialized);
    TEST_ASSERT(RTTBridge_GetValidatedRTTRegion(&address, &region_size) == 0);
    TEST_ASSERT(address == _stub_local_base);
    TEST_ASSERT(region_size == _stub_find_region_size);

    RTTBridge_SetRunning(true);
    TEST_ASSERT(RTTBridge_IsRunning());
    RTTBridge_IncrementPolls();
    RTTBridge_IncrementErrors();
    TEST_ASSERT(RTTBridge_GetState()->polls_count == 1u);
    TEST_ASSERT(RTTBridge_GetState()->errors_count == 1u);
    TEST_ASSERT(RTTBridge_CheckUpBufferChannel(0u) == 0);
    TEST_ASSERT(RTTBridge_CheckDownBufferChannel(0u) == 0);
    TEST_ASSERT(RTTBridge_GetBytesInBuffer(0u) == 3);
    TEST_ASSERT(RTTBridge_ReadUpBufferNoLock(0u, buffer, sizeof(buffer)) == 2);
    TEST_ASSERT(memcmp(buffer, "rt", 2u) == 0);
    TEST_ASSERT(RTTBridge_WriteDownBufferNoLock(0u, "abc", 3u) == 3);
    TEST_ASSERT(RTTBridge_WaitForRTTInitialized(address, 1u, 1u) == 0);
    TEST_ASSERT(RTTBridge_WaitForRTTUpChannelReady(address, region_size, 0u, 1u, 1u) == 0);

    RTTBridge_Status();
    RTTBridge_Cleanup();
    TEST_ASSERT(!RTTBridge_GetState()->initialized);
    return 0;
}

static int _TestBridgeChannelFailureContracts(void) {
    RTTBridge_Config_t config;
    volatile sig_atomic_t run_flag;
    uintptr_t address;
    size_t    region_size;
    char      buffer[8];

    _ResetStubs();
    _stub_check_init_result = -1;
    TEST_ASSERT(RTTBridge_WaitForRTTInitialized(1u, 1u, 1u) == -1);
    TEST_ASSERT(_stub_sleep_count == 1u);

    _ResetStubs();
    _stub_check_region_result = -1;
    TEST_ASSERT(RTTBridge_WaitForRTTUpChannelReady(1u, 16u, 0u, 1u, 1u) == -1);
    TEST_ASSERT(_stub_sleep_count == 1u);

    _ResetStubs();
    RTTBridge_Cleanup();
    run_flag = 1;
    memset(&config, 0, sizeof(config));
    config.device_path = "stub";
    config.run_flag = &run_flag;
    TEST_ASSERT(RTTBridge_Init(&config) == 0);
    TEST_ASSERT(RTTBridge_GetValidatedRTTRegion(&address, &region_size) == 0);

    _stub_check_up_result = -1;
    TEST_ASSERT(RTTBridge_CheckUpBufferChannel(0u) == -1);
    TEST_ASSERT(RTTBridge_GetBytesInBuffer(0u) == -1);
    TEST_ASSERT(RTTBridge_ReadUpBufferNoLock(0u, buffer, sizeof(buffer)) == -1);
    _stub_check_up_result = 0;

    _stub_check_down_result = -1;
    TEST_ASSERT(RTTBridge_CheckDownBufferChannel(0u) == -1);
    TEST_ASSERT(RTTBridge_WriteDownBufferNoLock(0u, "abc", 3u) == -1);
    _stub_check_down_result = 0;

    _stub_bytes_in_buffer = (unsigned)INT_MAX + 1u;
    TEST_ASSERT(RTTBridge_GetBytesInBuffer(0u) == -1);
    _stub_bytes_in_buffer = 3u;

    _stub_read_result = (unsigned)INT_MAX + 1u;
    TEST_ASSERT(RTTBridge_ReadUpBufferNoLock(0u, buffer, sizeof(buffer)) == -1);
    _stub_read_result = 2u;

    _stub_write_override_enabled = true;
    _stub_write_result = (unsigned)INT_MAX + 1u;
    TEST_ASSERT(RTTBridge_WriteDownBufferNoLock(0u, "abc", 3u) == -1);
    _stub_write_override_enabled = false;

    TEST_ASSERT(RTTBridge_ReadUpBufferNoLock(0u, buffer, (size_t)INT_MAX + 1u) == -1);
    TEST_ASSERT(RTTBridge_WriteDownBufferNoLock(0u, "abc", (size_t)INT_MAX + 1u) == -1);

    _stub_check_region_result = -1;
    _stub_find_region_size = 0x800u;
    TEST_ASSERT(RTTBridge_GetValidatedRTTRegion(&address, &region_size) == 0);
    TEST_ASSERT(region_size == _stub_find_region_size);

    RTTBridge_Cleanup();
    return 0;
}

static int _TestRegionSearchValidationContracts(void) {
    uintptr_t address;
    size_t    region_size;

    _ResetRegionSearchState();
    RTTBridge_Cleanup();
    TEST_ASSERT(RTTMem_InstallBackend(&_stub_backend) == 0);
    TEST_ASSERT(RTTBridgeRegion_SetupMemoryMapping(NULL, 0u, 0u) == -1);

    address = 0u;
    region_size = 0u;
    TEST_ASSERT(RTTBridgeRegion_FindValid(NULL, &region_size) == -2);
    TEST_ASSERT(RTTBridgeRegion_FindValid(&address, NULL) == -2);

    _ResetRegionSearchState();
    _stub_size = 0u;
    TEST_ASSERT(RTTBridgeRegion_FindValid(&address, &region_size) == -2);

    _ResetRegionSearchState();
    _bridge_state.config.rtt_address = _stub_base - 1u;
    TEST_ASSERT(RTTBridgeRegion_FindValid(&address, &region_size) == -2);

    _ResetRegionSearchState();
    _bridge_state.config.rtt_address = _stub_base + _stub_size;
    TEST_ASSERT(RTTBridgeRegion_FindValid(&address, &region_size) == -2);

    _ResetRegionSearchState();
    _bridge_state.config.rtt_region_size = SEGGER_RTT__CB_OFF_A_UP - 1u;
    TEST_ASSERT(RTTBridgeRegion_FindValid(&address, &region_size) == -2);

    _ResetRegionSearchState();
    _bridge_state.config.rtt_address = _stub_base + (_stub_size / 2u);
    _bridge_state.config.rtt_region_size = (_stub_size / 2u) + 1u;
    TEST_ASSERT(RTTBridgeRegion_FindValid(&address, &region_size) == -2);

    _ResetRegionSearchState();
    _stub_force_map_base_failure = true;
    TEST_ASSERT(RTTBridgeRegion_FindValid(&address, &region_size) == -2);

    _ResetRegionSearchState();
    _stub_force_search_base_failure = true;
    _bridge_state.config.rtt_address = _stub_base + SEGGER_RTT__CB_OFF_A_UP;
    _bridge_state.config.rtt_region_size = SEGGER_RTT__CB_OFF_A_UP;
    TEST_ASSERT(RTTBridgeRegion_FindValid(&address, &region_size) == -2);

    _ResetRegionSearchState();
    _stub_find_result = -1;
    TEST_ASSERT(RTTBridgeRegion_FindValid(&address, &region_size) == -1);
    TEST_ASSERT(_stub_find_call_count == 1u);

    _ResetRegionSearchState();
    _bridge_state.config.rtt_address = _stub_base + 0x100u;
    _bridge_state.config.rtt_region_size = 0x300u;
    _stub_find_region_size = 0x200u;
    TEST_ASSERT(RTTBridgeRegion_FindValid(&address, &region_size) == 0);
    TEST_ASSERT(address == _stub_local_base + 0x100u);
    TEST_ASSERT(region_size == 0x200u);
    TEST_ASSERT(_stub_find_call_count == 1u);

    RTTBridge_Cleanup();
    return 0;
}

int main(void) {
    TEST_RUN(_TestMemoryDispatcher);
    TEST_RUN(_TestBridgeInvalidStateContracts);
    TEST_RUN(_TestBridgeInitAbortCleansBackend);
    TEST_RUN(_TestBridgeInitFailureAndRetryPaths);
    TEST_RUN(_TestBridgeInitializedOperations);
    TEST_RUN(_TestBridgeChannelFailureContracts);
    TEST_RUN(_TestRegionSearchValidationContracts);
    return 0;
}

/*************************** End of file ****************************/
