/*********************************************************************
*                                                                    *
*                Copyright (C) 2023 xrTest Inc.                      *
*                      All rights reserved                           *
*                                                                    *
**********************************************************************
----------------------------------------------------------------------
File    : RTTMemory.c
Purpose : Backend-agnostic memory access dispatcher for RTT operations
Author  : songwenshuai <songwenshuai@gmail.com>
---------------------------END-OF-HEADER------------------------------
*/

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include <stdint.h>
#include <stdbool.h>

#include "RTTMemory.h"

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

static const RTTMem_Backend_t *_backend = NULL;
static bool                    _reset_on_init = false;

/*********************************************************************
*
*       Public functions - Backend management
*
**********************************************************************
*/

/*********************************************************************
*
*       RTTMem_InstallBackend()
*
*  Function description
*    Install a memory backend implementation.
*    Must be called before RTTMem_Init().
*
*  Parameters
*    backend  Pointer to backend operations structure
*
*  Return value
*    0   Success
*   -1   Invalid parameter
*/
int RTTMem_InstallBackend(const RTTMem_Backend_t *backend) {
    if (backend == NULL) {
        return -1;
    }

    /* Validate required operations */
    if (backend->init == NULL ||
        backend->cleanup == NULL ||
        backend->get_base == NULL ||
        backend->get_size == NULL ||
        backend->to_local_address == NULL) {
        return -1;
    }

    _backend = backend;
    return 0;
}

/*********************************************************************
*
*       RTTMem_GetBackend()
*
*  Function description
*    Get the currently installed backend.
*
*  Return value
*    Pointer to backend operations, or NULL if not installed
*/
const RTTMem_Backend_t *RTTMem_GetBackend(void) {
    return _backend;
}

/*********************************************************************
*
*       RTTMem_GetBackendName()
*
*  Function description
*    Get the name of the currently installed backend.
*
*  Return value
*    Backend name string, or "none" if not installed
*/
const char *RTTMem_GetBackendName(void) {
    if (_backend == NULL || _backend->name == NULL) {
        return "none";
    }
    return _backend->name;
}

/*********************************************************************
*
*       RTTMem_SetResetOnInit()
*
*  Function description
*    Configure whether a backend should reset its memory object during init.
*/
void RTTMem_SetResetOnInit(bool enabled) {
    _reset_on_init = enabled;
}

/*********************************************************************
*
*       RTTMem_GetResetOnInit()
*
*  Function description
*    Return the configured reset-on-init option.
*/
bool RTTMem_GetResetOnInit(void) {
    return _reset_on_init;
}

/*********************************************************************
*
*       RTTMem_InstallDefaultBackend()
*
*  Function description
*    Install the default backend based on compile-time configuration.
*    - RTTMEM_USE_MEMSHM: Use POSIX shared memory backend
*    - RTTMEM_USE_SMEM:   Use SharedMem driver backend (default)
*
*  Return value
*    0   Success
*   -1   Failed
*/
int RTTMem_InstallDefaultBackend(void) {
#if defined(RTTMEM_USE_MEMSHM)
    return RTTMem_InstallBackend(RTTMem_Backend_MEMSHM());
#else
    return RTTMem_InstallBackend(RTTMem_Backend_SMEM());
#endif
}

/*********************************************************************
*
*       Public functions - Initialization
*
**********************************************************************
*/

/*********************************************************************
*
*       RTTMem_Init()
*
*  Function description
*    Initialize RTT memory interface using installed backend.
*    Uses auto-detection for backend base address and size.
*
*  Parameters
*    device_path  Path to device or shared memory name
*
*  Return value
*    0   Success
*   -1   Failed to initialize or no backend installed
*/
int RTTMem_Init(const char *device_path) {
    return RTTMem_InitEx(device_path, 0, 0);
}

/*********************************************************************
*
*       RTTMem_InitEx()
*
*  Function description
*    Initialize RTT memory interface with explicit parameters.
*
*  Parameters
*    path  Path to device or shared memory name
*    base  Backend base address (0 for auto-detect)
*    size  Memory size (0 for auto-detect)
*
*  Return value
*    0   Success
*   -1   Failed to initialize or no backend installed
*/
int RTTMem_InitEx(const char *path, uint64_t base, size_t size) {
    if (_backend == NULL) {
        return -1;
    }
    return _backend->init(path, base, size);
}

/*********************************************************************
*
*       RTTMem_Cleanup()
*
*  Function description
*    Cleanup RTT memory interface.
*/
void RTTMem_Cleanup(void) {
    if (_backend != NULL) {
        _backend->cleanup();
    }
}

/*********************************************************************
*
*       Public functions - Memory Information
*
**********************************************************************
*/

/*********************************************************************
*
*       RTTMem_GetBackendBase()
*
*  Function description
*    Get the backend base address of the RTT memory region.
*
*  Return value
*    Backend base address, or 0 if not initialized
*/
uint64_t RTTMem_GetBackendBase(void) {
    if (_backend == NULL) {
        return 0;
    }
    return _backend->get_base();
}

/*********************************************************************
*
*       RTTMem_GetMappedSize()
*
*  Function description
*    Get the mapped size of the RTT memory region.
*
*  Return value
*    Size in bytes, or 0 if not initialized
*/
size_t RTTMem_GetMappedSize(void) {
    if (_backend == NULL) {
        return 0;
    }
    return _backend->get_size();
}

/*********************************************************************
*
*       RTTMem_ToLocalAddress()
*
*  Function description
*    Convert a backend address to an address that can be accessed
*    directly by the current process.
*
*  Parameters
*    address  Backend address
*    size     Number of bytes that must be accessible from address
*
*  Return value
*    Local process address, or 0 on error
*/
uintptr_t RTTMem_ToLocalAddress(uint64_t address, size_t size) {
    if (_backend == NULL) {
        return 0;
    }

    return _backend->to_local_address(address, size);
}

/*************************** End of file ****************************/
