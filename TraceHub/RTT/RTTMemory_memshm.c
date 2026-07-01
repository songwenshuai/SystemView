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
File    : RTTMemory_memshm.c
Purpose : MEMSHM (host shared memory) backend for RTT memory operations
          Enables host-based simulation testing without hardware.
---------------------------END-OF-HEADER------------------------------
*/

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#if defined(_WIN32)
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
  #endif
  #include <windows.h>
#else
  #include <unistd.h>
  #include <sys/mman.h>
  #include <fcntl.h>
  #include <sys/stat.h>
#endif

#include "RTTMemory.h"
#include "SEGGER_RTT.h"

/*********************************************************************
*
*       Defines
*
**********************************************************************
*/

#if (SEGGER_RTT_MAX_NUM_UP_BUFFERS == 0) || (SEGGER_RTT_MAX_NUM_DOWN_BUFFERS == 0)
  #error "MEMSHM backend requires at least one RTT up/down buffer"
#endif

#define RTTMEM_BUFFER_PAIRS \
        ((SEGGER_RTT_MAX_NUM_UP_BUFFERS > SEGGER_RTT_MAX_NUM_DOWN_BUFFERS) ? \
         SEGGER_RTT_MAX_NUM_UP_BUFFERS : SEGGER_RTT_MAX_NUM_DOWN_BUFFERS)
#define RTTMEM_RTT_SIZE             SEGGER_RTT_REQUIRED_MEM_SIZE_FOR_BUFFER_PAIRS(RTTMEM_BUFFER_PAIRS)
#define RTTMEM_CALCULATED_SIZE      (RTTMEM_RTT_SIZE + SEGGER_RTT_SPINLOCK_SW_SIZE)

#ifndef RTTMEM_MEMSHM_DEFAULT_SIZE
  #define RTTMEM_MEMSHM_DEFAULT_SIZE  RTTMEM_CALCULATED_SIZE
#endif

#ifndef RTTMEM_MEMSHM_DEFAULT_BASE
  #define RTTMEM_MEMSHM_DEFAULT_BASE  (0x10000000) /* Simulated backend base address */
#endif

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

#if defined(_WIN32)
static HANDLE    _mapping_handle = NULL;
#else
static int       _shm_fd         = -1;
#endif
static void     *_mapped_base    = NULL;
static uint64_t  _virt_base      = 0;      /* Simulated backend base address */
static size_t    _mapped_size    = 0;
static char      _shm_name[256]  = {0};
static int       _is_creator     = 0;      /* 1 if we created the shm object */

/*********************************************************************
*
*       Static functions
*
**********************************************************************
*/

static int _memshm_normalize_name(const char *path, char *name, size_t name_size) {
    if (path == NULL || path[0] == '\0' || name == NULL || name_size == 0u) {
        return -1;
    }

#if defined(_WIN32)
    size_t src;
    size_t dst;

    if (strncmp(path, "Global\\", 7u) == 0 || strncmp(path, "Local\\", 6u) == 0) {
        if (snprintf(name, name_size, "%s", path) >= (int)name_size) {
            return -1;
        }
        return 0;
    }
    if (snprintf(name, name_size, "Local\\") >= (int)name_size) {
        return -1;
    }
    dst = strlen(name);
    src = (path[0] == '/') ? 1u : 0u;
    if (path[src] == '\0') {
        return -1;
    }
    while (path[src] != '\0') {
        if (dst + 1u >= name_size) {
            return -1;
        }
        name[dst++] = (path[src] == '/' || path[src] == '\\') ? '_' : path[src];
        src++;
    }
    name[dst] = '\0';
    return 0;
#else
    if (path[0] != '/') {
        return -1;
    }
    if (snprintf(name, name_size, "%s", path) >= (int)name_size) {
        return -1;
    }
    return 0;
#endif
}

#if defined(_WIN32)
static int _memshm_map_windows(const char *name, size_t size, bool reset_on_init) {
    DWORD size_high;
    DWORD size_low;

#if SIZE_MAX > UINT64_MAX
    if (size > UINT64_MAX) {
        return -1;
    }
#endif
    size_high = (DWORD)(((uint64_t)size) >> 32);
    size_low = (DWORD)(((uint64_t)size) & 0xffffffffu);

    _mapping_handle = CreateFileMappingA(INVALID_HANDLE_VALUE,
                                         NULL,
                                         PAGE_READWRITE,
                                         size_high,
                                         size_low,
                                         name);
    if (_mapping_handle == NULL) {
        fprintf(stderr, "MEMSHM: CreateFileMapping failed: %lu\n", GetLastError());
        return -1;
    }
    _is_creator = (GetLastError() != ERROR_ALREADY_EXISTS);

    _mapped_base = MapViewOfFile(_mapping_handle,
                                 FILE_MAP_ALL_ACCESS,
                                 0u,
                                 0u,
                                 size);
    if (_mapped_base == NULL) {
        fprintf(stderr, "MEMSHM: MapViewOfFile failed: %lu\n", GetLastError());
        CloseHandle(_mapping_handle);
        _mapping_handle = NULL;
        _is_creator = 0;
        return -1;
    }
    if (_is_creator || reset_on_init) {
        memset(_mapped_base, 0, size);
    }
    return 0;
}
#else
static int _memshm_map_posix(const char *name, size_t *size, bool reset_on_init) {
    struct stat st;

    if (reset_on_init && shm_unlink(name) == -1 && errno != ENOENT) {
        fprintf(stderr, "MEMSHM: shm_unlink failed: %s\n", strerror(errno));
        return -1;
    }

    _shm_fd = shm_open(name, O_RDWR, 0666);
    if (_shm_fd == -1) {
        _shm_fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL, 0666);
        if (_shm_fd == -1) {
            fprintf(stderr, "MEMSHM: shm_open failed: %s\n", strerror(errno));
            return -1;
        }
        _is_creator = 1;
        if (ftruncate(_shm_fd, (off_t)*size) == -1) {
            fprintf(stderr, "MEMSHM: ftruncate failed: %s\n", strerror(errno));
            close(_shm_fd);
            shm_unlink(name);
            _shm_fd = -1;
            _is_creator = 0;
            return -1;
        }
    } else {
        _is_creator = 0;
    }

    if (fstat(_shm_fd, &st) == -1) {
        fprintf(stderr, "MEMSHM: fstat failed: %s\n", strerror(errno));
        close(_shm_fd);
        if (_is_creator) {
            shm_unlink(name);
        }
        _shm_fd = -1;
        _is_creator = 0;
        return -1;
    }
    if ((st.st_size < 0) || ((size_t)st.st_size < *size)) {
        fprintf(stderr, "MEMSHM: existing shared memory object is too small: %zu bytes required, %zu bytes found\n",
                *size, (st.st_size < 0) ? 0u : (size_t)st.st_size);
        close(_shm_fd);
        if (_is_creator) {
            shm_unlink(name);
        }
        _shm_fd = -1;
        _is_creator = 0;
        return -1;
    }
    *size = (size_t)st.st_size;

    _mapped_base = mmap(NULL, *size, PROT_READ | PROT_WRITE, MAP_SHARED, _shm_fd, 0);
    if (_mapped_base == MAP_FAILED) {
        fprintf(stderr, "MEMSHM: mmap failed: %s\n", strerror(errno));
        close(_shm_fd);
        if (_is_creator) {
            shm_unlink(name);
        }
        _shm_fd = -1;
        _mapped_base = NULL;
        _is_creator = 0;
        return -1;
    }
    if (_is_creator || reset_on_init) {
        memset(_mapped_base, 0, *size);
    }
    return 0;
}
#endif

/*********************************************************************
*
*       _memshm_init()
*
*  Function description
*    Initialize MEMSHM backend using host shared memory.
*
*  Parameters
*    path  Shared memory name (e.g., "/rtt_shm")
*    base  Simulated backend base address (0 for default)
*    size  Memory region size (0 for default)
*
*  Return value
*    0   Success
*   -1   Failed
*
*  Notes
*    If the shared memory object already exists, it will be opened.
*    If it doesn't exist, it will be created with the specified size.
*    The object is not unlinked during normal cleanup so independent simulator
*    processes keep resolving the same shared memory object across restarts.
*/
static int _memshm_init(const char *path, uint64_t base, size_t size) {
    bool reset_on_init;

    if (_mapped_base != NULL) {
        return 0;  /* Already initialized */
    }

    if (_memshm_normalize_name(path, _shm_name, sizeof(_shm_name)) != 0) {
        return -1;
    }

    /* Use defaults if not specified */
    if (base == 0) {
        base = RTTMEM_MEMSHM_DEFAULT_BASE;
    }
    if (size == 0) {
        size = RTTMEM_MEMSHM_DEFAULT_SIZE;
    }

    reset_on_init = RTTMem_GetResetOnInit();
#if defined(_WIN32)
    if (_memshm_map_windows(_shm_name, size, reset_on_init) != 0) {
        return -1;
    }
#else
    if (_memshm_map_posix(_shm_name, &size, reset_on_init) != 0) {
        return -1;
    }
#endif

    _virt_base = base;
    _mapped_size = size;

    return 0;
}

/*********************************************************************
*
*       _memshm_cleanup()
*
*  Function description
*    Cleanup MEMSHM backend.
*/
static void _memshm_cleanup(void) {
    if (_mapped_base != NULL && _mapped_size > 0) {
#if defined(_WIN32)
        UnmapViewOfFile(_mapped_base);
#else
        munmap(_mapped_base, _mapped_size);
#endif
        _mapped_base = NULL;
    }

#if defined(_WIN32)
    if (_mapping_handle != NULL) {
        CloseHandle(_mapping_handle);
        _mapping_handle = NULL;
    }
#else
    if (_shm_fd >= 0) {
        close(_shm_fd);
        _shm_fd = -1;
    }
#endif

    _is_creator = 0;

    _virt_base = 0;
    _mapped_size = 0;
    _shm_name[0] = '\0';
}

/*********************************************************************
*
*       _memshm_get_base()
*
*  Function description
*    Get simulated backend base address.
*
*  Return value
*    Simulated backend base address
*/
static uint64_t _memshm_get_base(void) {
    return (uint64_t)_virt_base;
}

/*********************************************************************
*
*       _memshm_get_size()
*
*  Function description
*    Get mapped size.
*
*  Return value
*    Size in bytes
*/
static size_t _memshm_get_size(void) {
    return _mapped_size;
}

/*********************************************************************
*
*       _memshm_to_local_address()
*
*  Function description
*    Convert a simulated address to the local mapped address.
*
*  Parameters
*    addr  Simulated address
*    size  Number of bytes that must be accessible
*
*  Return value
*    Local process address, or 0 on error
*/
static uintptr_t _memshm_to_local_address(uint64_t addr, size_t size) {
    uint64_t offset64;
    size_t   offset;

    if (_mapped_base == NULL || _mapped_size == 0) {
        return 0;
    }
    if (addr < _virt_base) {
        return 0;
    }

    offset64 = addr - _virt_base;
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

static const RTTMem_Backend_t _memshm_backend = {
    .name      = "memshm",
    .init      = _memshm_init,
    .cleanup   = _memshm_cleanup,
    .get_base  = _memshm_get_base,
    .get_size  = _memshm_get_size,
    .to_local_address = _memshm_to_local_address,
};

/*********************************************************************
*
*       Public functions
*
**********************************************************************
*/

/*********************************************************************
*
*       RTTMem_Backend_MEMSHM()
*
*  Function description
*    Get the MEMSHM backend implementation.
*
*  Return value
*    Pointer to backend operations structure
*/
const RTTMem_Backend_t *RTTMem_Backend_MEMSHM(void) {
    return &_memshm_backend;
}

/*************************** End of file ****************************/
