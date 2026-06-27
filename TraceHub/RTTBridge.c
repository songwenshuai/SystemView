/*********************************************************************
*                                                                    *
*                Copyright (C) 2023 xrTest Inc.                      *
*                      All rights reserved                           *
*                                                                    *
**********************************************************************
----------------------------------------------------------------------
File    : RTTBridge.c
Purpose : RTT Bridge core state and configuration management
Author  : songwenshuai <songwenshuai@gmail.com>
---------------------------END-OF-HEADER------------------------------
*/

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <limits.h>

#include "SYS.h"
#include "RTTMemory.h"
#include "RTTBridge.h"
#include "Log.h"
#include "SEGGER_RTT.h"

/*********************************************************************
*
*       Defines
*
**********************************************************************
*/

#ifndef RTT_CB_SEARCH_RETRY_INTERVAL_MS
  #define RTT_CB_SEARCH_RETRY_INTERVAL_MS   1000    /* Retry interval in ms */
#endif

#ifndef RTT_CB_SEARCH_MAX_RETRIES
  #define RTT_CB_SEARCH_MAX_RETRIES         0       /* 0 = infinite retries */
#endif

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

static RTTBridge_State_t _bridge_state;
static SYS_Mutex         _bridge_lock = PTHREAD_MUTEX_INITIALIZER;

/*********************************************************************
*
*       Static functions
*
**********************************************************************
*/

/*********************************************************************
*
*       _Bridge_LockState()
*
*  Function description
*    Lock bridge module state.
*/
static void _Bridge_LockState(void) {
    SYS_MutexLock(&_bridge_lock);
}

/*********************************************************************
*
*       _Bridge_UnlockState()
*
*  Function description
*    Unlock bridge state after _Bridge_LockState() succeeds.
*/
static void _Bridge_UnlockState(void) {
    SYS_MutexUnlock(&_bridge_lock);
}

/*********************************************************************
*
*       _SetupMemoryMapping()
*
*  Function description
*    Initialize and configure memory mapping for RTT buffer access.
*
*  Parameters
*    device_path      Device path or shared memory name
*    backend_address  Backend base address for explicit mapping
*    map_size         Mapping size for explicit mapping
*
*  Return value
*    0   Success
*   -1   Failed to initialize or map memory
*/
static int _SetupMemoryMapping(const char *device_path, uint64_t backend_address, size_t map_size) {
    int ret;
    //
    // Validate parameters
    //
    if (device_path == NULL) {
        Log_Error("Invalid parameters: device_path is NULL\n");
        return -1;
    }
    //
    // Install default memory backend based on compile-time configuration
    //
    RTTMem_SetResetOnInit(_bridge_state.config.reset_memory);
    ret = RTTMem_InstallDefaultBackend();
    if (ret != 0) {
        Log_Error("Failed to install memory backend\n");
        return -1;
    }
    Log_Print("Memory backend installed: %s\n", RTTMem_GetBackendName());
    //
    // Initialize RTT shared memory interface
    //
    Log_Print("Initializing RTT shared memory interface\n");
    Log_Print("Device/SHM path: %s\n", device_path);

    ret = RTTMem_InitEx(device_path, backend_address, map_size);
    if (ret != 0) {
        Log_Error("Failed to initialize RTT shared memory interface\n");
        return -1;
    }
    _Bridge_LockState();
    _bridge_state.memory_initialized = true;
    _Bridge_UnlockState();

    Log_Print("RTT memory interface initialized successfully\n");
    Log_Print("Backend base address: 0x%016" PRIx64 "\n", RTTMem_GetBackendBase());
    Log_Print("Mapped size: %zu bytes\n", RTTMem_GetMappedSize());

    return 0;
}

/*********************************************************************
*
*       _CleanupMemoryMapping()
*
*  Function description
*    Cleanup memory mappings and resources.
*/
static void _CleanupMemoryMapping(void) {
    bool memory_initialized;

    _Bridge_LockState();
    memory_initialized = _bridge_state.memory_initialized;
    _bridge_state.memory_initialized = false;
    _Bridge_UnlockState();

    if (!memory_initialized) {
        return;
    }
    //
    // Cleanup RTT memory interface
    //
    Log_Print("Cleaning up RTT memory interface\n");
    RTTMem_Cleanup();
}

/*********************************************************************
*
*       _FindValidRTTRegion()
*
*  Function description
*    Search the configured mapped backend range for a valid RTT control block.
*
*  Parameters
*    address      Receives local RTT control block address
*    region_size  Receives remaining mapped region size from address
*
*  Return value
*    0   Valid RTT control block found
*   -1   Control block was not found in a valid mapping
*   -2   Mapping or search range is invalid
*/
static int _FindValidRTTRegion(uintptr_t *address, size_t *region_size) {
    uintptr_t map_base;
    uintptr_t search_base;
    uintptr_t candidate;
    uint64_t  backend_base;
    uint64_t  search_backend_address;
    uint64_t  search_offset64;
    size_t    mapped_size;
    size_t    search_offset;
    size_t    search_size;

    if ((address == NULL) || (region_size == NULL)) {
        Log_Error("Invalid RTT search output parameter\n");
        return -2;
    }

    mapped_size = RTTMem_GetMappedSize();
    backend_base = RTTMem_GetBackendBase();
    search_backend_address = (_bridge_state.config.rtt_address != 0u) ?
                             _bridge_state.config.rtt_address :
                             backend_base;

    if (mapped_size == 0u) {
        Log_Error("RTT mapped region is empty\n");
        return -2;
    }
    if (search_backend_address < backend_base) {
        Log_Error("RTT search address is before mapped backend base\n");
        return -2;
    }
    search_offset64 = search_backend_address - backend_base;
    if (search_offset64 > SIZE_MAX) {
        Log_Error("RTT search offset is outside local size range\n");
        return -2;
    }
    search_offset = (size_t)search_offset64;
    if (search_offset >= mapped_size) {
        Log_Error("RTT search address is outside mapped backend range\n");
        return -2;
    }

    search_size = (_bridge_state.config.rtt_region_size != 0u) ?
                  _bridge_state.config.rtt_region_size :
                  (mapped_size - search_offset);
    if (search_size < SEGGER_RTT__CB_OFF_A_UP) {
        Log_Error("RTT search region is too small\n");
        return -2;
    }
    if (search_size > (mapped_size - search_offset)) {
        Log_Error("RTT search region is outside mapped backend range\n");
        return -2;
    }

    map_base = RTTMem_ToLocalAddress(backend_base, mapped_size);
    if ((map_base == 0u) || (mapped_size > (size_t)(UINTPTR_MAX - map_base))) {
        Log_Error("Failed to convert mapped RTT region to local address range\n");
        return -2;
    }
    search_base = RTTMem_ToLocalAddress(search_backend_address, search_size);
    if ((search_base == 0u) || (search_size > (size_t)(UINTPTR_MAX - search_base))) {
        Log_Error("Failed to convert RTT search address to local mapping\n");
        return -2;
    }

    candidate = search_base;
    if (SEGGER_RTT_FindValidControlBlock(&candidate, search_size, region_size) != 0) {
        return -1;
    }

    *address = candidate;
    return 0;
}

/*********************************************************************
*
*       Public functions
*
**********************************************************************
*/

/*********************************************************************
*
*       RTTBridge_EnsureRTTInitialized()
*/
int RTTBridge_EnsureRTTInitialized(uintptr_t address, size_t region_size, unsigned num_channels) {
    return SEGGER_RTT_EnsureInitEx(address, region_size, num_channels);
}

/*********************************************************************
*
*       RTTBridge_WaitForRTTInitialized()
*/
int RTTBridge_WaitForRTTInitialized(uintptr_t address, unsigned timeout_ms, unsigned retry_interval_ms) {
    unsigned elapsed_ms;

    if ((address == 0u) || (retry_interval_ms == 0u)) {
        return -1;
    }

    elapsed_ms = 0u;
    while (SEGGER_RTT_CheckInit(address) != 0) {
        if ((timeout_ms > 0u) && (elapsed_ms >= timeout_ms)) {
            return -1;
        }
        SYS_Sleep(retry_interval_ms);
        if ((timeout_ms > 0u) && (retry_interval_ms > (UINT_MAX - elapsed_ms))) {
            elapsed_ms = timeout_ms;
        } else {
            elapsed_ms += retry_interval_ms;
        }
    }

    return 0;
}

/*********************************************************************
*
*       RTTBridge_WaitForRTTUpChannelReady()
*/
int RTTBridge_WaitForRTTUpChannelReady(uintptr_t address, size_t region_size,
                                       unsigned channel, unsigned timeout_ms,
                                       unsigned retry_interval_ms) {
    unsigned elapsed_ms;

    if ((address == 0u) || (region_size == 0u) || (retry_interval_ms == 0u)) {
        return -1;
    }

    elapsed_ms = 0u;
    while ((SEGGER_RTT_CheckRegion(address, region_size) != 0) ||
           (SEGGER_RTT_CheckUpBuffer(address, region_size, channel) != 0)) {
        if ((timeout_ms > 0u) && (elapsed_ms >= timeout_ms)) {
            return -1;
        }
        SYS_Sleep(retry_interval_ms);
        if ((timeout_ms > 0u) && (retry_interval_ms > (UINT_MAX - elapsed_ms))) {
            elapsed_ms = timeout_ms;
        } else {
            elapsed_ms += retry_interval_ms;
        }
    }

    return 0;
}

/*********************************************************************
*
*       RTTBridge_Init()
*/
int RTTBridge_Init(RTTBridge_Config_t *config) {
    int ret;
    uintptr_t address;
    size_t    region_size = 0;
    unsigned retry_count = 0;
    unsigned elapsed_ms = 0;
    unsigned sleep_ms;

    if ((config == NULL) ||
        ((config->rtt_region_size != 0u) && (config->rtt_region_size < SEGGER_RTT__CB_OFF_A_UP))) {
        return -1;
    }
    //
    // Initialize state
    //
    _Bridge_LockState();
    if (_bridge_state.initialized || _bridge_state.memory_initialized) {
        _Bridge_UnlockState();
        return -1;
    }
    memset(&_bridge_state, 0, sizeof(_bridge_state));
    memcpy(&_bridge_state.config, config, sizeof(RTTBridge_Config_t));
    _Bridge_UnlockState();

    Log_Print("Initializing RTT Bridge\n");
    Log_Print("RTT region: addr=0x%016" PRIx64 ", size=0x%lx (%zu bytes)\n",
              config->rtt_address,
              (unsigned long)config->rtt_region_size,
              config->rtt_region_size);
    //
    // Setup memory mapping via configured backend
    //
    ret = _SetupMemoryMapping(config->device_path, config->rtt_address, config->rtt_region_size);
    if (ret != 0) {
        Log_Error("Failed to setup memory mapping\n");
        RTTBridge_Cleanup();
        return -1;
    }
    // Find RTT control block with retry mechanism.
    // This allows the bridge to wait for target (RTOS) to initialize RTT CB.
    //
    Log_Print("Searching for RTT control block (will retry if not found)...\n");

    while (1) {
        if ((config->run_flag != NULL) && (*config->run_flag == 0)) {
            Log_Warn("RTT control block search aborted by run flag\n");
            RTTBridge_Cleanup();
            return -3;
        }

        ret = _FindValidRTTRegion(&address, &region_size);

        if (ret == -2) {
            Log_Error("RTT control block search stopped because the mapped range is invalid\n");
            RTTBridge_Cleanup();
            return -1;
        }

        if (ret == 0) {
            //
            // Found RTT control block
            //
            _Bridge_LockState();
            _bridge_state.rtt_cb_address = address;
            _bridge_state.rtt_region_size = region_size;
            _bridge_state.initialized = true;
            _Bridge_UnlockState();
            Log_Print("Found RTT control block at local address 0x%" PRIxPTR "\n",
                      address);
            return 0;
        }
        //
        // RTT CB not found, check if we should retry
        //
        retry_count++;

        if (RTT_CB_SEARCH_MAX_RETRIES > 0 && retry_count >= RTT_CB_SEARCH_MAX_RETRIES) {
            Log_Error("Failed to find RTT control block after %u retries\n", retry_count);
            RTTBridge_Cleanup();
            return -2;
        }
        if ((config->rtt_search_timeout_ms > 0u) &&
            (elapsed_ms >= config->rtt_search_timeout_ms)) {
            Log_Error("Failed to find RTT control block after %u ms\n", elapsed_ms);
            RTTBridge_Cleanup();
            return -2;
        }
        //
        // Wait and retry - target may not have initialized RTT yet
        //
        if (retry_count == 1) {
            Log_Print("RTT control block not found, waiting for target initialization...\n");
        } else if (retry_count % 10 == 0) {
            Log_Print("Still waiting for RTT control block (retry %u)...\n", retry_count);
        }

        sleep_ms = RTT_CB_SEARCH_RETRY_INTERVAL_MS;
        if (config->rtt_search_timeout_ms > 0u) {
            unsigned remaining_ms;

            remaining_ms = config->rtt_search_timeout_ms - elapsed_ms;
            if (sleep_ms > remaining_ms) {
                sleep_ms = remaining_ms;
            }
        }
        SYS_Sleep(sleep_ms);
        if (sleep_ms > (UINT_MAX - elapsed_ms)) {
            elapsed_ms = UINT_MAX;
        } else {
            elapsed_ms += sleep_ms;
        }
    }
}

/*********************************************************************
*
*       RTTBridge_Cleanup()
*/
void RTTBridge_Cleanup(void) {
    _Bridge_LockState();
    _bridge_state.running = false;
    _bridge_state.initialized = false;
    _Bridge_UnlockState();

    _CleanupMemoryMapping();

    _Bridge_LockState();
    memset(&_bridge_state, 0, sizeof(_bridge_state));
    _Bridge_UnlockState();
}

/*********************************************************************
*
*       RTTBridge_GetState()
*/
const RTTBridge_State_t* RTTBridge_GetState(void) {
    return &_bridge_state;
}

/*********************************************************************
*
*       RTTBridge_GetValidatedRTTRegion()
*/
int RTTBridge_GetValidatedRTTRegion(uintptr_t *address, size_t *region_size) {
    uintptr_t current_address;
    size_t    current_region_size;
    bool      recovered;

    if (address == NULL) {
        return -1;
    }

    recovered = false;
    _Bridge_LockState();
    if (!_bridge_state.initialized) {
        _Bridge_UnlockState();
        return -1;
    }
    current_address = _bridge_state.rtt_cb_address;
    current_region_size = _bridge_state.rtt_region_size;
    if ((current_address == 0u) ||
        (current_region_size == 0u) ||
        (SEGGER_RTT_CheckRegion(current_address, current_region_size) != 0)) {
        if (_FindValidRTTRegion(&current_address, &current_region_size) != 0) {
            _Bridge_UnlockState();
            return -1;
        }
        _bridge_state.rtt_cb_address = current_address;
        _bridge_state.rtt_region_size = current_region_size;
        recovered = true;
    }
    _Bridge_UnlockState();

    if (recovered) {
        Log_Print("Recovered RTT control block at local address 0x%" PRIxPTR "\n",
                  current_address);
    }

    *address = current_address;
    if (region_size != NULL) {
        *region_size = current_region_size;
    }
    return 0;
}

/*********************************************************************
*
*       RTTBridge_CheckUpBufferChannel()
*/
int RTTBridge_CheckUpBufferChannel(unsigned channel) {
    uintptr_t address;
    size_t    region_size;

    if (RTTBridge_GetValidatedRTTRegion(&address, &region_size) != 0) {
        return -1;
    }
    if (SEGGER_RTT_CheckUpBuffer(address, region_size, channel) != 0) {
        return -1;
    }

    return 0;
}

/*********************************************************************
*
*       RTTBridge_CheckDownBufferChannel()
*/
int RTTBridge_CheckDownBufferChannel(unsigned channel) {
    uintptr_t address;
    size_t    region_size;

    if (RTTBridge_GetValidatedRTTRegion(&address, &region_size) != 0) {
        return -1;
    }
    if (SEGGER_RTT_CheckDownBuffer(address, region_size, channel) != 0) {
        return -1;
    }

    return 0;
}

/*********************************************************************
*
*       RTTBridge_GetBytesInBuffer()
*/
int RTTBridge_GetBytesInBuffer(unsigned channel) {
    uintptr_t address;
    size_t    region_size;
    unsigned  bytes_in_buffer;

    if (RTTBridge_GetValidatedRTTRegion(&address, &region_size) != 0) {
        return -1;
    }
    if (SEGGER_RTT_CheckUpBuffer(address, region_size, channel) != 0) {
        return -1;
    }
    bytes_in_buffer = SEGGER_RTT_GetBytesInBuffer(address, channel);
    if (bytes_in_buffer > (unsigned)INT_MAX) {
        return -1;
    }
    return (int)bytes_in_buffer;
}

/*********************************************************************
*
*       RTTBridge_ReadUpBufferNoLock()
*/
int RTTBridge_ReadUpBufferNoLock(unsigned channel, void *buffer, size_t buffer_size) {
    uintptr_t address;
    size_t    region_size;
    unsigned  bytes_read;

    if ((buffer == NULL) || (buffer_size > (size_t)INT_MAX)) {
        return -1;
    }
    if (RTTBridge_GetValidatedRTTRegion(&address, &region_size) != 0) {
        return -1;
    }
    if (SEGGER_RTT_CheckUpBuffer(address, region_size, channel) != 0) {
        return -1;
    }
    bytes_read = SEGGER_RTT_ReadUpBufferNoLock(address, channel, buffer, (unsigned)buffer_size);
    if (bytes_read > (unsigned)INT_MAX) {
        return -1;
    }
    return (int)bytes_read;
}

/*********************************************************************
*
*       RTTBridge_WriteDownBufferNoLock()
*/
int RTTBridge_WriteDownBufferNoLock(unsigned channel, const void *buffer, size_t num_bytes) {
    uintptr_t address;
    size_t    region_size;
    unsigned  bytes_written;

    if ((buffer == NULL) || (num_bytes > (size_t)INT_MAX)) {
        return -1;
    }
    if (RTTBridge_GetValidatedRTTRegion(&address, &region_size) != 0) {
        return -1;
    }
    if (SEGGER_RTT_CheckDownBuffer(address, region_size, channel) != 0) {
        return -1;
    }
    bytes_written = SEGGER_RTT_WriteDownBufferNoLock(address, channel, buffer, (unsigned)num_bytes);
    if (bytes_written > (unsigned)INT_MAX) {
        return -1;
    }
    return (int)bytes_written;
}

/*********************************************************************
*
*       RTTBridge_IsRunning()
*/
bool RTTBridge_IsRunning(void) {
    bool running;

    _Bridge_LockState();
    running = _bridge_state.initialized && _bridge_state.running;
    _Bridge_UnlockState();
    return running;
}

/*********************************************************************
*
*       RTTBridge_SetRunning()
*/
void RTTBridge_SetRunning(bool running) {
    _Bridge_LockState();
    if (!_bridge_state.initialized) {
        _Bridge_UnlockState();
        return;
    }

    _bridge_state.running = running;
    _Bridge_UnlockState();
}

/*********************************************************************
*
*       RTTBridge_GetLogFile()
*/
const char* RTTBridge_GetLogFile(void) {
    const char *log_file;

    _Bridge_LockState();
    if (!_bridge_state.initialized) {
        _Bridge_UnlockState();
        return NULL;
    }

    log_file = _bridge_state.config.log_file;
    _Bridge_UnlockState();
    return log_file;
}

/*********************************************************************
*
*       RTTBridge_GetLogFileHandle()
*/
FILE* RTTBridge_GetLogFileHandle(void) {
    FILE *log_file_handle;

    _Bridge_LockState();
    if (!_bridge_state.initialized) {
        _Bridge_UnlockState();
        return NULL;
    }

    log_file_handle = _bridge_state.config.log_file_handle;
    _Bridge_UnlockState();
    return log_file_handle;
}

/*********************************************************************
*
*       RTTBridge_IncrementPolls()
*/
void RTTBridge_IncrementPolls(void) {
    _Bridge_LockState();
    if (!_bridge_state.initialized) {
        _Bridge_UnlockState();
        return;
    }

    _bridge_state.polls_count++;
    _Bridge_UnlockState();
}

/*********************************************************************
*
*       RTTBridge_IncrementErrors()
*/
void RTTBridge_IncrementErrors(void) {
    _Bridge_LockState();
    if (!_bridge_state.initialized) {
        _Bridge_UnlockState();
        return;
    }

    _bridge_state.errors_count++;
    _Bridge_UnlockState();
}

/*********************************************************************
*
*       RTTBridge_Status()
*/
void RTTBridge_Status(void) {
    _Bridge_LockState();
    if (!_bridge_state.initialized) {
        _Bridge_UnlockState();
        printf("RTT Bridge Status: not initialized\n");
        return;
    }

    printf("RTT Bridge Status:\n");
    printf("  RTT CB Address: 0x%" PRIxPTR "\n", _bridge_state.rtt_cb_address);
    printf("  RTT Region Size: %zu\n", _bridge_state.rtt_region_size);
    printf("  Running:        %s\n", _bridge_state.running ? "yes" : "no");
    printf("  Polls Count:    %u\n", _bridge_state.polls_count);
    printf("  Errors Count:   %u\n", _bridge_state.errors_count);
    _Bridge_UnlockState();
}

/*************************** End of file ****************************/
