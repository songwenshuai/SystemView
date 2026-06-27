/*********************************************************************
*                             CineLogic                              *
*                      RTT Trace and Debug Bridge                    *
**********************************************************************
*                                                                    *
*                    (c) 2023 - 2026 CineLogic                       *
*                                                                    *
*                  Support: wenshuaisong@gmail.com                   *
*                                                                    *
**********************************************************************
*                                                                    *
*       CineLogic TraceHub * RTT trace and debug bridge              *
*                                                                    *
**********************************************************************
*                                                                    *
* All rights reserved.                                               *
*                                                                    *
* CineLogic strongly recommends to not make any changes              *
* to or modify the source code of this software in order to stay     *
* compatible with the SharedMem and RTT data path.                   *
*                                                                    *
* Redistribution and use in source and binary forms, with or         *
* without modification, are permitted provided that the following    *
* condition is met:                                                  *
*                                                                    *
* o Redistributions of source code must retain the above copyright   *
*   notice, this condition and the following disclaimer.             *
*                                                                    *
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND             *
* CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,        *
* INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF           *
* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE           *
* DISCLAIMED. IN NO EVENT SHALL CINELOGIC BE LIABLE FOR              *
* ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR           *
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT  *
* OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;    *
* OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF      *
* LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT          *
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE  *
* USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH   *
* DAMAGE.                                                            *
*                                                                    *
**********************************************************************
----------------------------------------------------------------------
File    : RTTMemory_smem.c
Purpose : SMEM (SharedMem driver) backend for RTT memory operations
---------------------------END-OF-HEADER------------------------------
*/

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include "Log.h"
#include "RTTMemory.h"
#include "SharedMem.h"

/*********************************************************************
*
*       Types
*
**********************************************************************
*/

/**
 * Error codes returned by smem functions
 */
typedef enum {
    SMEM_SUCCESS               =  0,
    SMEM_ERROR_OPEN_FAILED     = -1,
    SMEM_ERROR_INVALID_HANDLE  = -2,
    SMEM_ERROR_READ_FAILED     = -3,
    SMEM_ERROR_WRITE_FAILED    = -4,
    SMEM_ERROR_INVALID_PARAMS  = -5,
    SMEM_ERROR_MMAP_FAILED     = -6,
    SMEM_ERROR_MUNMAP_FAILED   = -7,
    SMEM_ERROR_IOCTL_FAILED    = -8,
} smem_error_t;

/**
 * Internal device handle structure
 */
struct smem_handle {
    int      fd;            /* Device file descriptor */
    uint64_t backend_base;  /* Backend base address */
    size_t   size;          /* Region size */
};

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

static struct smem_handle *_smem_handle = NULL;
static void              *_mapped_base  = NULL;
static void              *_mmap_base    = NULL;
static uint64_t           _backend_base = 0;
static size_t             _mapped_size  = 0;
static size_t             _mmap_size    = 0;

/*********************************************************************
*
*       Static functions - SMEM device library
*
**********************************************************************
*/

/*********************************************************************
*
*       _smem_lib_open()
*
*  Function description
*    Open the SharedMem device.
*
*  Parameters
*    device_path  Path to the device (e.g., "/dev/shared_mem0")
*
*  Return value
*    Handle to the device, or NULL on failure
*/
static struct smem_handle *_smem_lib_open(const char *device_path) {
    int fd;
    struct smem_handle *handle;
    sharedmem_addr_t backend_base;
    sharedmem_size_t mem_size;

    if (!device_path) {
        return NULL;
    }

    fd = open(device_path, O_RDWR);
    if (fd < 0) {
        perror("smem_open: open SharedMem device failed");
        return NULL;
    }

    handle = (struct smem_handle *)malloc(sizeof(struct smem_handle));
    if (!handle) {
        close(fd);
        return NULL;
    }

    handle->fd = fd;

    if (ioctl(fd, SHAREDMEM_GET_PHYS_ADDR, &backend_base) < 0) {
        perror("smem_open: SHAREDMEM_GET_PHYS_ADDR failed");
        close(fd);
        free(handle);
        return NULL;
    }

    if (ioctl(fd, SHAREDMEM_GET_MEM_SIZE, &mem_size) < 0) {
        perror("smem_open: SHAREDMEM_GET_MEM_SIZE failed");
        close(fd);
        free(handle);
        return NULL;
    }
    if (mem_size == 0u || mem_size > (sharedmem_size_t)SIZE_MAX) {
        fprintf(stderr, "smem_open: invalid SharedMem region size: %llu\n",
                (unsigned long long)mem_size);
        close(fd);
        free(handle);
        return NULL;
    }

    handle->backend_base = (uint64_t)backend_base;
    handle->size = (size_t)mem_size;

    return handle;
}

/*********************************************************************
*
*       _smem_lib_close()
*
*  Function description
*    Close the SharedMem device.
*
*  Parameters
*    handle  Device handle returned by _smem_lib_open
*
*  Return value
*    Error code
*/
static smem_error_t _smem_lib_close(struct smem_handle *handle) {
    if (!handle) {
        return SMEM_ERROR_INVALID_HANDLE;
    }

    if (close(handle->fd) < 0) {
        free(handle);
        return SMEM_ERROR_INVALID_HANDLE;
    }

    free(handle);
    return SMEM_SUCCESS;
}

/*********************************************************************
*
*       _smem_lib_get_backend_base()
*
*  Function description
*    Get the backend base address of the region.
*
*  Parameters
*    handle        Device handle
*    backend_base  Pointer to store the backend base address
*
*  Return value
*    Error code
*/
static smem_error_t _smem_lib_get_backend_base(struct smem_handle *handle,
                                               uint64_t *backend_base) {
    if (!handle || !backend_base) {
        return SMEM_ERROR_INVALID_PARAMS;
    }

    *backend_base = handle->backend_base;
    return SMEM_SUCCESS;
}

/*********************************************************************
*
*       _smem_lib_get_size()
*
*  Function description
*    Get the size of the memory region.
*
*  Parameters
*    handle  Device handle
*    size    Pointer to store the size
*
*  Return value
*    Error code
*/
static smem_error_t _smem_lib_get_size(struct smem_handle *handle,
                                        size_t *size) {
    if (!handle || !size) {
        return SMEM_ERROR_INVALID_PARAMS;
    }

    *size = handle->size;
    return SMEM_SUCCESS;
}

/*********************************************************************
*
*       _smem_lib_mmap()
*
*  Function description
*    Map a portion of the memory region into user space.
*
*  Parameters
*    handle  Device handle
*    offset  Offset from the backend base address
*    size    Size of region to map in bytes
*    addr    Pointer to store the mapped address
*
*  Return value
*    Error code
*/
static smem_error_t _smem_lib_mmap(struct smem_handle *handle,
                                    uint64_t offset, size_t size,
                                    void **addr) {
    void *mapped_addr;
    off_t mmap_offset;

    if (!handle || !addr || size == 0) {
        return SMEM_ERROR_INVALID_PARAMS;
    }

    if ((offset > (uint64_t)handle->size) || (size > (size_t)((uint64_t)handle->size - offset))) {
        return SMEM_ERROR_INVALID_PARAMS;
    }
    mmap_offset = (off_t)offset;
    if ((uint64_t)mmap_offset != offset) {
        return SMEM_ERROR_INVALID_PARAMS;
    }

    mapped_addr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED,
                        handle->fd, mmap_offset);
    if (mapped_addr == MAP_FAILED) {
        perror("smem_mmap: mmap failed");
        return SMEM_ERROR_MMAP_FAILED;
    }

    *addr = mapped_addr;
    return SMEM_SUCCESS;
}

/*********************************************************************
*
*       _smem_page_size()
*
*  Function description
*    Return the host page size used for mmap offset alignment.
*/
static size_t _smem_page_size(void) {
    long page_size;

    page_size = sysconf(_SC_PAGE_SIZE);
    if (page_size <= 0) {
        return 0u;
    }
    return (size_t)page_size;
}

/*********************************************************************
*
*       _smem_prepare_mapping()
*
*  Function description
*    Normalize the requested backend range and calculate the page-aligned
*    mmap range required to expose it.
*
*  Return value
*    0   Success
*   -1   Invalid range or alignment calculation failed
*/
static int _smem_prepare_mapping(struct smem_handle *handle,
                                 uint64_t requested_base,
                                 size_t requested_size,
                                 uint64_t *mapped_backend_base,
                                 size_t *mapped_size,
                                 uint64_t *mmap_offset,
                                 size_t *mmap_size,
                                 size_t *mmap_delta) {
    uint64_t request_offset64;
    uint64_t aligned_offset64;
    size_t   request_offset;
    size_t   page_size;
    size_t   delta;

    if (handle == NULL || mapped_backend_base == NULL || mapped_size == NULL ||
        mmap_offset == NULL || mmap_size == NULL || mmap_delta == NULL ||
        handle->size == 0u) {
        return -1;
    }

    if (requested_base == 0u) {
        requested_base = handle->backend_base;
    }
    if (requested_base < handle->backend_base) {
        return -1;
    }
    request_offset64 = requested_base - handle->backend_base;
    if (request_offset64 >= (uint64_t)handle->size || request_offset64 > SIZE_MAX) {
        return -1;
    }
    request_offset = (size_t)request_offset64;

    if (requested_size == 0u) {
        requested_size = handle->size - request_offset;
    }
    if (requested_size == 0u || requested_size > (handle->size - request_offset)) {
        return -1;
    }

    page_size = _smem_page_size();
    if (page_size == 0u) {
        return -1;
    }

    aligned_offset64 = request_offset64 - (request_offset64 % (uint64_t)page_size);
    delta = (size_t)(request_offset64 - aligned_offset64);
    if (requested_size > (SIZE_MAX - delta)) {
        return -1;
    }
    if ((delta + requested_size) > (handle->size - (size_t)aligned_offset64)) {
        return -1;
    }

    *mapped_backend_base = requested_base;
    *mapped_size = requested_size;
    *mmap_offset = aligned_offset64;
    *mmap_size = delta + requested_size;
    *mmap_delta = delta;
    return 0;
}

/*********************************************************************
*
*       _smem_lib_munmap()
*
*  Function description
*    Unmap a previously mapped memory region.
*
*  Parameters
*    addr  Address of the mapped region
*    size  Size of the mapped region in bytes
*
*  Return value
*    Error code
*/
static smem_error_t _smem_lib_munmap(void *addr, size_t size) {
    if (!addr || size == 0) {
        return SMEM_ERROR_INVALID_PARAMS;
    }

    if (munmap(addr, size) < 0) {
        perror("smem_munmap: munmap failed");
        return SMEM_ERROR_MUNMAP_FAILED;
    }

    return SMEM_SUCCESS;
}

/*********************************************************************
*
*       Static functions - RTT memory backend
*
**********************************************************************
*/

/*********************************************************************
*
*       _smem_init()
*
*  Function description
*    Initialize SMEM backend.
*
*  Parameters
*    path  Device path (e.g., "/dev/shared_mem0")
*    base  Backend base address to map, 0 selects the driver base
*    size  Number of bytes to map, 0 maps to the end of the driver region
*
*  Return value
*    0   Success
*   -1   Failed
*/
static int _smem_init(const char *path, uint64_t base, size_t size) {
    smem_error_t ret;
    uint64_t     mapped_backend_base;
    uint64_t     mmap_offset;
    size_t       driver_size;
    size_t       page_delta;

    if (_smem_handle != NULL) {
        return 0;  /* Already initialized */
    }

    _smem_handle = _smem_lib_open(path);
    if (_smem_handle == NULL) {
        Log_Error("SMEM: failed to open or query SharedMem device: %s\n",
                  path != NULL ? path : "(null)");
        return -1;
    }

    ret = _smem_lib_get_backend_base(_smem_handle, &mapped_backend_base);
    if (ret != SMEM_SUCCESS) {
        Log_Error("SMEM: failed to get backend base: %d\n", ret);
        _smem_lib_close(_smem_handle);
        _smem_handle = NULL;
        return -1;
    }

    ret = _smem_lib_get_size(_smem_handle, &driver_size);
    if (ret != SMEM_SUCCESS) {
        Log_Error("SMEM: failed to get mapped size: %d\n", ret);
        _smem_lib_close(_smem_handle);
        _smem_handle = NULL;
        return -1;
    }

    if (_smem_prepare_mapping(_smem_handle, base, size,
                              &_backend_base, &_mapped_size,
                              &mmap_offset, &_mmap_size,
                              &page_delta) != 0) {
        Log_Error("SMEM: requested mapping is outside SharedMem region "
                  "(driver_base=0x%016llx, driver_size=%zu, requested_base=0x%016llx, requested_size=%zu)\n",
                  (unsigned long long)mapped_backend_base,
                  driver_size,
                  (unsigned long long)base,
                  size);
        _smem_lib_close(_smem_handle);
        _smem_handle = NULL;
        _backend_base = 0;
        _mapped_size = 0;
        _mmap_size = 0;
        return -1;
    }

    ret = _smem_lib_mmap(_smem_handle, mmap_offset, _mmap_size, &_mmap_base);
    if (ret != SMEM_SUCCESS) {
        Log_Error("SMEM: failed to mmap SharedMem region: %d\n", ret);
        _smem_lib_close(_smem_handle);
        _smem_handle = NULL;
        _backend_base = 0;
        _mapped_size = 0;
        _mmap_size = 0;
        return -1;
    }
    _mapped_base = (uint8_t *)_mmap_base + page_delta;

    return 0;
}

/*********************************************************************
*
*       _smem_cleanup()
*
*  Function description
*    Cleanup SMEM backend.
*/
static void _smem_cleanup(void) {
    if (_mmap_base != NULL && _mmap_size > 0) {
        _smem_lib_munmap(_mmap_base, _mmap_size);
        _mmap_base = NULL;
        _mapped_base = NULL;
    }

    if (_smem_handle != NULL) {
        _smem_lib_close(_smem_handle);
        _smem_handle = NULL;
    }

    _backend_base = 0;
    _mapped_size = 0;
    _mmap_size = 0;
}

/*********************************************************************
*
*       _smem_get_base()
*
*  Function description
*    Get backend base address.
*
*  Return value
*    Backend base address
*/
static uint64_t _smem_get_base(void) {
    return _backend_base;
}

/*********************************************************************
*
*       _smem_get_size()
*
*  Function description
*    Get mapped size.
*
*  Return value
*    Size in bytes
*/
static size_t _smem_get_size(void) {
    return _mapped_size;
}

/*********************************************************************
*
*       _smem_to_local_address()
*
*  Function description
*    Convert a backend address to the local mmap address.
*
*  Parameters
*    addr  Backend address
*    size  Number of bytes that must be accessible
*
*  Return value
*    Local process address, or 0 on error
*/
static uintptr_t _smem_to_local_address(uint64_t addr, size_t size) {
    uint64_t offset64;
    size_t   offset;

    if (_mapped_base == NULL || _mapped_size == 0) {
        return 0;
    }
    if (addr < _backend_base) {
        return 0;
    }

    offset64 = addr - _backend_base;
    if (offset64 > SIZE_MAX) {
        return 0;
    }
    offset = (size_t)offset64;
    if (offset >= _mapped_size) {
        return 0;
    }
    if (size > (_mapped_size - offset)) {
        return 0;
    }

    return (uintptr_t)((uint8_t *)_mapped_base + offset);
}

/*********************************************************************
*
*       Static const data - Backend definition
*
**********************************************************************
*/

static const RTTMem_Backend_t _smem_backend = {
    .name      = "smem",
    .init      = _smem_init,
    .cleanup   = _smem_cleanup,
    .get_base  = _smem_get_base,
    .get_size  = _smem_get_size,
    .to_local_address = _smem_to_local_address,
};

/*********************************************************************
*
*       Public functions
*
**********************************************************************
*/

/*********************************************************************
*
*       RTTMem_Backend_SMEM()
*
*  Function description
*    Get the SMEM backend implementation.
*
*  Return value
*    Pointer to backend operations structure
*/
const RTTMem_Backend_t *RTTMem_Backend_SMEM(void) {
    return &_smem_backend;
}

/*************************** End of file ****************************/
