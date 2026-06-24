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
File    : SharedMem_Lib.c
Purpose : C library implementation for the SharedMem debug mmap driver.
---------------------------END-OF-HEADER------------------------------
*/

#include "SharedMem_Lib.h"
#include "SharedMem.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

/*********************************************************************
*
*       Types
*
**********************************************************************
*/

/*********************************************************************
*
*       SHAREDMEM_CONTEXT
*
*  Description
*    Internal device handle.
*/
struct SHAREDMEM_CONTEXT {
  int      File;       // Device file descriptor.
  uint64_t PhysAddr;   // Physical base address.
  uint64_t Size;       // Region size.
};

/*********************************************************************
*
*       Static functions
*
**********************************************************************
*/

/*********************************************************************
*
*       _IsRangeValid()
*
*  Function description
*    Checks whether an offset range is fully inside a mapped shared
*    memory region.
*
*  Parameters
*    RegionSize  Size of the mapped region.
*    Off         Offset from the region base.
*    NumBytes    Number of bytes in the range.
*
*  Return value
*    != 0  Range is valid.
*    == 0  Range is invalid.
*/
static int _IsRangeValid(uint64_t RegionSize, uint64_t Off, size_t NumBytes) {
  if (NumBytes == 0u) {
    return 0;
  }
  if (Off > RegionSize) {
    return 0;
  }
  return (uint64_t)NumBytes <= (RegionSize - Off);
}

/*********************************************************************
*
*       _GetPageSize()
*
*  Function description
*    Returns the system page size used for mmap alignment checks through
*    an output parameter.
*
*  Parameters
*    pPageSize  Destination for the page size in bytes.
*
*  Return value
*    != 0  Page size was returned.
*    == 0  Page size query failed.
*/
static int _GetPageSize(size_t* pPageSize) {
  long PageSize;

  if (pPageSize == NULL) {
    return 0;
  }

  PageSize = sysconf(_SC_PAGESIZE);
  if (PageSize <= 0) {
    return 0;
  }

  *pPageSize = (size_t)PageSize;
  return 1;
}

/*********************************************************************
*
*       _IsPageAligned()
*
*  Function description
*    Checks whether a value is aligned to the system page size.
*
*  Parameters
*    Value  Value to check.
*
*  Return value
*    != 0  Value is page-aligned.
*    == 0  Value is not page-aligned.
*/
static int _IsPageAligned(uint64_t Value) {
  size_t PageSize;

  if (!_GetPageSize(&PageSize)) {
    return 0;
  }

  return (Value % PageSize) == 0u;
}

/*********************************************************************
*
*       Public API functions
*
**********************************************************************
*/

/*********************************************************************
*
*       SHAREDMEM_StrError()
*
*  Function description
*    Returns a human-readable error string for an error code.
*
*  Parameters
*    Error  Error code.
*
*  Return value
*    Pointer to a static error string.
*/
const char* SHAREDMEM_StrError(SHAREDMEM_ERROR Error) {
  switch (Error) {
  case SHAREDMEM_SUCCESS:
    return "Success";
  case SHAREDMEM_ERROR_OPEN_FAILED:
    return "Failed to open device";
  case SHAREDMEM_ERROR_INVALID_HANDLE:
    return "Invalid handle";
  case SHAREDMEM_ERROR_READ_FAILED:
    return "Read operation failed";
  case SHAREDMEM_ERROR_WRITE_FAILED:
    return "Write operation failed";
  case SHAREDMEM_ERROR_INVALID_PARAMS:
    return "Invalid parameters";
  case SHAREDMEM_ERROR_MAP_FAILED:
    return "Memory mapping failed";
  case SHAREDMEM_ERROR_UNMAP_FAILED:
    return "Memory unmapping failed";
  case SHAREDMEM_ERROR_IOCTL_FAILED:
    return "IOCTL operation failed";
  default:
    return "Unknown error";
  }
}

/*********************************************************************
*
*       SHAREDMEM_Open()
*
*  Function description
*    Opens a SharedMem character device and reads its region metadata.
*
*  Parameters
*    sDevice  Path to the device node.
*
*  Return value
*    != NULL  Handle to the opened device.
*    == NULL  Open or metadata query failed.
*/
SHAREDMEM_HANDLE SHAREDMEM_Open(const char* sDevice) {
  int File;
  SHAREDMEM_HANDLE hMem;

  if (sDevice == NULL) {
    return NULL;
  }

  File = open(sDevice, O_RDWR);
  if (File < 0) {
    perror("SHAREDMEM_Open: open failed");
    return NULL;
  }

  hMem = malloc(sizeof(struct SHAREDMEM_CONTEXT));
  if (hMem == NULL) {
    close(File);
    return NULL;
  }

  hMem->File = File;
  if (ioctl(File, SHAREDMEM_GET_PHYS_ADDR, &hMem->PhysAddr) < 0) {
    perror("SHAREDMEM_Open: SHAREDMEM_GET_PHYS_ADDR failed");
    close(File);
    free(hMem);
    return NULL;
  }

  if (ioctl(File, SHAREDMEM_GET_MEM_SIZE, &hMem->Size) < 0) {
    perror("SHAREDMEM_Open: SHAREDMEM_GET_MEM_SIZE failed");
    close(File);
    free(hMem);
    return NULL;
  }

  return hMem;
}

/*********************************************************************
*
*       SHAREDMEM_Close()
*
*  Function description
*    Closes a SharedMem device handle.
*
*  Parameters
*    hMem  Handle returned by SHAREDMEM_Open().
*
*  Return value
*    SHAREDMEM_SUCCESS               Handle closed.
*    SHAREDMEM_ERROR_INVALID_HANDLE  Invalid handle or close failed.
*/
SHAREDMEM_ERROR SHAREDMEM_Close(SHAREDMEM_HANDLE hMem) {
  if (hMem == NULL) {
    return SHAREDMEM_ERROR_INVALID_HANDLE;
  }

  if (close(hMem->File) < 0) {
    free(hMem);
    return SHAREDMEM_ERROR_INVALID_HANDLE;
  }

  free(hMem);
  return SHAREDMEM_SUCCESS;
}

/*********************************************************************
*
*       SHAREDMEM_GetPhysAddr()
*
*  Function description
*    Returns the physical base address associated with a device handle.
*
*  Parameters
*    hMem       SharedMem device handle.
*    pPhysAddr  Destination for the physical base address.
*
*  Return value
*    SHAREDMEM_SUCCESS               Address returned.
*    SHAREDMEM_ERROR_INVALID_PARAMS  Invalid parameter.
*/
SHAREDMEM_ERROR SHAREDMEM_GetPhysAddr(SHAREDMEM_HANDLE hMem, uint64_t* pPhysAddr) {
  if ((hMem == NULL) || (pPhysAddr == NULL)) {
    return SHAREDMEM_ERROR_INVALID_PARAMS;
  }

  *pPhysAddr = hMem->PhysAddr;
  return SHAREDMEM_SUCCESS;
}

/*********************************************************************
*
*       SHAREDMEM_GetSize()
*
*  Function description
*    Returns the size of the mapped physical region.
*
*  Parameters
*    hMem   SharedMem device handle.
*    pSize  Destination for the region size.
*
*  Return value
*    SHAREDMEM_SUCCESS               Size returned.
*    SHAREDMEM_ERROR_INVALID_PARAMS  Invalid parameter or size_t overflow.
*/
SHAREDMEM_ERROR SHAREDMEM_GetSize(SHAREDMEM_HANDLE hMem, size_t* pSize) {
  if ((hMem == NULL) || (pSize == NULL)) {
    return SHAREDMEM_ERROR_INVALID_PARAMS;
  }

  if (hMem->Size > (uint64_t)SIZE_MAX) {
    return SHAREDMEM_ERROR_INVALID_PARAMS;
  }

  *pSize = (size_t)hMem->Size;
  return SHAREDMEM_SUCCESS;
}

/*********************************************************************
*
*       SHAREDMEM_Read()
*
*  Function description
*    Reads bytes from the physical shared-memory region through ioctl.
*
*  Parameters
*    hMem      SharedMem device handle.
*    Off       Offset from the region base.
*    pBuffer   Destination buffer.
*    NumBytes  Number of bytes to read.
*
*  Return value
*    SHAREDMEM_SUCCESS               Data read.
*    SHAREDMEM_ERROR_INVALID_PARAMS  Invalid parameter or range.
*    SHAREDMEM_ERROR_READ_FAILED     ioctl read failed.
*/
SHAREDMEM_ERROR SHAREDMEM_Read(SHAREDMEM_HANDLE hMem, uint64_t Off, void* pBuffer, size_t NumBytes) {
  struct sharedmem_ioctl_data Data;

  if ((hMem == NULL) || (pBuffer == NULL) || (NumBytes == 0u)) {
    return SHAREDMEM_ERROR_INVALID_PARAMS;
  }
  if (!_IsRangeValid(hMem->Size, Off, NumBytes)) {
    return SHAREDMEM_ERROR_INVALID_PARAMS;
  }

  Data.address = Off;
  Data.size    = NumBytes;
  Data.data    = (sharedmem_user_ptr_t)(uintptr_t)pBuffer;
  if (ioctl(hMem->File, SHAREDMEM_IOCTL_READ, &Data) < 0) {
    perror("SHAREDMEM_Read: SHAREDMEM_IOCTL_READ failed");
    return SHAREDMEM_ERROR_READ_FAILED;
  }

  return SHAREDMEM_SUCCESS;
}

/*********************************************************************
*
*       SHAREDMEM_Write()
*
*  Function description
*    Writes bytes to the physical shared-memory region through ioctl.
*
*  Parameters
*    hMem      SharedMem device handle.
*    Off       Offset from the region base.
*    pBuffer   Source buffer.
*    NumBytes  Number of bytes to write.
*
*  Return value
*    SHAREDMEM_SUCCESS               Data written.
*    SHAREDMEM_ERROR_INVALID_PARAMS  Invalid parameter or range.
*    SHAREDMEM_ERROR_WRITE_FAILED    ioctl write failed.
*/
SHAREDMEM_ERROR SHAREDMEM_Write(SHAREDMEM_HANDLE hMem, uint64_t Off, const void* pBuffer, size_t NumBytes) {
  struct sharedmem_ioctl_data Data;

  if ((hMem == NULL) || (pBuffer == NULL) || (NumBytes == 0u)) {
    return SHAREDMEM_ERROR_INVALID_PARAMS;
  }
  if (!_IsRangeValid(hMem->Size, Off, NumBytes)) {
    return SHAREDMEM_ERROR_INVALID_PARAMS;
  }

  Data.address = Off;
  Data.size    = NumBytes;
  Data.data    = (sharedmem_user_ptr_t)(uintptr_t)pBuffer;
  if (ioctl(hMem->File, SHAREDMEM_IOCTL_WRITE, &Data) < 0) {
    perror("SHAREDMEM_Write: SHAREDMEM_IOCTL_WRITE failed");
    return SHAREDMEM_ERROR_WRITE_FAILED;
  }

  return SHAREDMEM_SUCCESS;
}

/*********************************************************************
*
*       SHAREDMEM_Map()
*
*  Function description
*    Maps a page-aligned subrange of the physical shared-memory region.
*
*  Parameters
*    hMem      SharedMem device handle.
*    Off       Page-aligned offset from the region base.
*    NumBytes  Page-aligned mapping size.
*    ppMem     Destination for the mapped user address.
*
*  Return value
*    SHAREDMEM_SUCCESS               Region mapped.
*    SHAREDMEM_ERROR_INVALID_PARAMS  Invalid parameter, range, or alignment.
*    SHAREDMEM_ERROR_MAP_FAILED      mmap failed.
*/
SHAREDMEM_ERROR SHAREDMEM_Map(SHAREDMEM_HANDLE hMem, uint64_t Off, size_t NumBytes, void** ppMem) {
  void* pMem;

  if ((hMem == NULL) || (ppMem == NULL) || (NumBytes == 0u)) {
    return SHAREDMEM_ERROR_INVALID_PARAMS;
  }
  if (!_IsRangeValid(hMem->Size, Off, NumBytes)) {
    return SHAREDMEM_ERROR_INVALID_PARAMS;
  }
  if (!_IsPageAligned(Off) || !_IsPageAligned(NumBytes)) {
    return SHAREDMEM_ERROR_INVALID_PARAMS;
  }

  pMem = mmap(NULL, NumBytes, PROT_READ | PROT_WRITE, MAP_SHARED, hMem->File, Off);
  if (pMem == MAP_FAILED) {
    perror("SHAREDMEM_Map: mmap failed");
    return SHAREDMEM_ERROR_MAP_FAILED;
  }

  *ppMem = pMem;
  return SHAREDMEM_SUCCESS;
}

/*********************************************************************
*
*       SHAREDMEM_Unmap()
*
*  Function description
*    Unmaps a user mapping previously returned by SHAREDMEM_Map().
*
*  Parameters
*    pMem      Mapped user address.
*    NumBytes  Mapping size in bytes.
*
*  Return value
*    SHAREDMEM_SUCCESS                Region unmapped.
*    SHAREDMEM_ERROR_INVALID_PARAMS   Invalid parameter.
*    SHAREDMEM_ERROR_UNMAP_FAILED     munmap failed.
*/
SHAREDMEM_ERROR SHAREDMEM_Unmap(void* pMem, size_t NumBytes) {
  if ((pMem == NULL) || (NumBytes == 0u)) {
    return SHAREDMEM_ERROR_INVALID_PARAMS;
  }

  if (munmap(pMem, NumBytes) < 0) {
    perror("SHAREDMEM_Unmap: munmap failed");
    return SHAREDMEM_ERROR_UNMAP_FAILED;
  }

  return SHAREDMEM_SUCCESS;
}
