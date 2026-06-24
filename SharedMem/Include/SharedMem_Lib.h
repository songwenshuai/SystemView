/*********************************************************************
*                             CineLogic                              *
*                      Shared-memory Debug Tools                     *
**********************************************************************
*                                                                    *
*                    (c) 2023 - 2026 CineLogic                       *
*                                                                    *
*                  Support: wenshuaisong@gmail.com                   *
*                                                                    *
**********************************************************************
*                                                                    *
*       CineLogic SharedMem * Shared-memory debug support            *
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
File    : SharedMem_Lib.h
Purpose : C library interface for the SharedMem debug mmap driver.
---------------------------END-OF-HEADER------------------------------
*/

#ifndef SHAREDMEM_LIB_H
#define SHAREDMEM_LIB_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************
*
*       Types
*
**********************************************************************
*/

typedef enum {
  SHAREDMEM_SUCCESS              =  0,   // Operation succeeded.
  SHAREDMEM_ERROR_OPEN_FAILED    = -1,   // Failed to open device.
  SHAREDMEM_ERROR_INVALID_HANDLE = -2,   // Invalid handle.
  SHAREDMEM_ERROR_READ_FAILED    = -3,   // Read operation failed.
  SHAREDMEM_ERROR_WRITE_FAILED   = -4,   // Write operation failed.
  SHAREDMEM_ERROR_INVALID_PARAMS = -5,   // Invalid parameters.
  SHAREDMEM_ERROR_MAP_FAILED     = -6,   // Memory mapping failed.
  SHAREDMEM_ERROR_UNMAP_FAILED   = -7,   // Memory unmapping failed.
  SHAREDMEM_ERROR_IOCTL_FAILED   = -8    // IOCTL operation failed.
} SHAREDMEM_ERROR;

typedef struct SHAREDMEM_CONTEXT* SHAREDMEM_HANDLE;

/*********************************************************************
*
*       API functions
*
**********************************************************************
*/

SHAREDMEM_HANDLE SHAREDMEM_Open       (const char* sDevice);
SHAREDMEM_ERROR  SHAREDMEM_Close      (SHAREDMEM_HANDLE hMem);
const char*      SHAREDMEM_StrError   (SHAREDMEM_ERROR Error);
SHAREDMEM_ERROR  SHAREDMEM_GetPhysAddr(SHAREDMEM_HANDLE hMem, uint64_t* pPhysAddr);
SHAREDMEM_ERROR  SHAREDMEM_GetSize    (SHAREDMEM_HANDLE hMem, size_t* pSize);
SHAREDMEM_ERROR  SHAREDMEM_Read       (SHAREDMEM_HANDLE hMem, uint64_t Off, void* pBuffer, size_t NumBytes);
SHAREDMEM_ERROR  SHAREDMEM_Write      (SHAREDMEM_HANDLE hMem, uint64_t Off, const void* pBuffer, size_t NumBytes);
SHAREDMEM_ERROR  SHAREDMEM_Map        (SHAREDMEM_HANDLE hMem, uint64_t Off, size_t NumBytes, void** ppMem);
SHAREDMEM_ERROR  SHAREDMEM_Unmap      (void* pMem, size_t NumBytes);

#ifdef __cplusplus
}
#endif

#endif /* SHAREDMEM_LIB_H */
