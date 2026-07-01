/*********************************************************************
*                             CineLogic                              *
*                      RTT Trace and Debug Bridge                    *
**********************************************************************
----------------------------------------------------------------------
File    : RTTBridgeRegion.c
Purpose : RTT bridge memory mapping and region recovery
---------------------------END-OF-HEADER------------------------------
*/

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include <inttypes.h>
#include <limits.h>

#include "RTTBridge_internal.h"
#include "Log.h"
#include "RTTMemory.h"
#include "SEGGER_RTT.h"

/*********************************************************************
*
*       Internal functions
*
**********************************************************************
*/

/*********************************************************************
*
*       RTTBridgeRegion_SetupMemoryMapping()
*
*  Function description
*    Initialize and configure memory mapping for RTT buffer access.
*
*  Parameters
*    device_path      Device path or shared memory name.
*    backend_address  Backend base address for explicit mapping.
*    map_size         Mapping size for explicit mapping.
*
*  Return value
*    0   Success.
*   -1   Failed to initialize or map memory.
*/
int RTTBridgeRegion_SetupMemoryMapping(const char *device_path, uint64_t backend_address, size_t map_size) {
    int ret;

    if (device_path == NULL) {
        Log_Error("Invalid parameters: device_path is NULL\n");
        return -1;
    }

    RTTMem_SetResetOnInit(_bridge_state.config.reset_memory);
    ret = RTTMem_InstallDefaultBackend();
    if (ret != 0) {
        Log_Error("Failed to install memory backend\n");
        return -1;
    }
    Log_Print("Memory backend installed: %s\n", RTTMem_GetBackendName());

    Log_Print("Initializing RTT shared memory interface\n");
    Log_Print("Device/SHM path: %s\n", device_path);

    ret = RTTMem_InitEx(device_path, backend_address, map_size);
    if (ret != 0) {
        Log_Error("Failed to initialize RTT shared memory interface\n");
        return -1;
    }
    RTTBridgeState_Lock();
    _bridge_state.memory_initialized = true;
    RTTBridgeState_Unlock();

    Log_Print("RTT memory interface initialized successfully\n");
    Log_Print("Backend base address: 0x%016" PRIx64 "\n", RTTMem_GetBackendBase());
    Log_Print("Mapped size: %zu bytes\n", RTTMem_GetMappedSize());

    return 0;
}

/*********************************************************************
*
*       RTTBridgeRegion_CleanupMemoryMapping()
*
*  Function description
*    Cleanup memory mappings and resources.
*/
void RTTBridgeRegion_CleanupMemoryMapping(void) {
    bool memory_initialized;

    RTTBridgeState_Lock();
    memory_initialized = _bridge_state.memory_initialized;
    _bridge_state.memory_initialized = false;
    RTTBridgeState_Unlock();

    if (!memory_initialized) {
        return;
    }

    Log_Print("Cleaning up RTT memory interface\n");
    RTTMem_Cleanup();
}

/*********************************************************************
*
*       RTTBridgeRegion_FindValid()
*
*  Function description
*    Search the configured mapped backend range for a valid RTT control block.
*
*  Parameters
*    address      Receives local RTT control block address.
*    region_size  Receives remaining mapped region size from address.
*
*  Return value
*    0   Valid RTT control block found.
*   -1   Control block was not found in a valid mapping.
*   -2   Mapping or search range is invalid.
*/
int RTTBridgeRegion_FindValid(uintptr_t *address, size_t *region_size) {
    uintptr_t map_base;
    uintptr_t search_base;
    PTR_ADDR  candidate;
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

    candidate = (PTR_ADDR)search_base;
    if (SEGGER_RTT_FindValidControlBlock(&candidate, search_size, region_size) != 0) {
        return -1;
    }
    if (candidate > (PTR_ADDR)UINTPTR_MAX) {
        Log_Error("Found RTT control block is outside local address range\n");
        return -2;
    }

    *address = (uintptr_t)candidate;
    return 0;
}

/*********************************************************************
*
*       RTTBridge_GetValidatedRTTRegion()
*
*  Function description
*    Get the current local mapped RTT control block address after
*    validating the current RTT region. If the stored address is invalid,
*    the configured mapped backend range is searched again.
*
*  Parameters
*    address      Pointer to store local mapped RTT control block address.
*    region_size  Optional pointer to store remaining mapped region size.
*
*  Return value
*    0   Success.
*   -1   RTT region is not initialized or invalid.
*/
int RTTBridge_GetValidatedRTTRegion(uintptr_t *address, size_t *region_size) {
    uintptr_t current_address;
    size_t    current_region_size;
    bool      recovered;

    if (address == NULL) {
        return -1;
    }

    recovered = false;
    RTTBridgeState_Lock();
    if (!_bridge_state.initialized) {
        RTTBridgeState_Unlock();
        return -1;
    }
    current_address = _bridge_state.rtt_cb_address;
    current_region_size = _bridge_state.rtt_region_size;
    if ((current_address == 0u) ||
        (current_region_size == 0u) ||
        (SEGGER_RTT_CheckRegion(current_address, current_region_size) != 0)) {
        if (RTTBridgeRegion_FindValid(&current_address, &current_region_size) != 0) {
            RTTBridgeState_Unlock();
            return -1;
        }
        _bridge_state.rtt_cb_address = current_address;
        _bridge_state.rtt_region_size = current_region_size;
        recovered = true;
    }
    RTTBridgeState_Unlock();

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

/*************************** End of file ****************************/
