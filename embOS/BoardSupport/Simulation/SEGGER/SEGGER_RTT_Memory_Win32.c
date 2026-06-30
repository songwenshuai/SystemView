/*********************************************************************
*                   (c) SEGGER Microcontroller GmbH                  *
*                        The Embedded Experts                        *
*                           www.segger.com                           *
**********************************************************************
*                                                                    *
*        SEGGER RTT * Real Time Transfer for embedded targets        *
*                  https://github.com/SEGGERMicro/RTT                *
*                                                                    *
**********************************************************************
----------------------------------------------------------------------
File    : SEGGER_RTT_Memory_Win32.c
Purpose : Win32 MEMSHM RTT memory ownership for embOS simulation.
---------------------------END-OF-HEADER------------------------------
*/

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#ifndef WIN32_LEAN_AND_MEAN
  #define WIN32_LEAN_AND_MEAN
#endif

#include "SEGGER_RTT_Memory_Int.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

static HANDLE           _hMapping;
static CRITICAL_SECTION _Lock;
static void            *_pMappedBase;
static size_t           _MappedSize;
static int              _IsCreator;
static int              _IsAtexitRegistered;
static int              _IsLockInitialized;

/*********************************************************************
*
*       Static code
*
**********************************************************************
*/

/*********************************************************************
*
*       _SEGGER_SIM_RTT_CopyString()
*
*  Function description
*    Copies a zero-terminated string into a fixed-size buffer.
*
*  Parameters
*    sDst     Destination buffer.
*    DstSize  Destination buffer size in bytes.
*    sSrc     Source string.
*
*  Return value
*    == 0  O.K.
*    != 0  Error.
*/
static int _SEGGER_SIM_RTT_CopyString(char *sDst, size_t DstSize, const char *sSrc) {
  int NumChars;

  NumChars = snprintf(sDst, DstSize, "%s", sSrc);
  if ((NumChars < 0) || ((size_t)NumChars >= DstSize)) {
    return -1;
  }
  return 0;
}

/*********************************************************************
*
*       _SEGGER_SIM_RTT_NormalizeName()
*
*  Function description
*    Converts the configured shared memory name into a Win32 file
*    mapping name.
*
*  Parameters
*    sPath     Configured shared memory name.
*    sName     Destination name buffer.
*    NameSize  Destination name buffer size in bytes.
*
*  Return value
*    == 0  O.K.
*    != 0  Error.
*/
static int _SEGGER_SIM_RTT_NormalizeName(const char *sPath, char *sName, size_t NameSize) {
  size_t SrcOff;
  size_t DstOff;

  if ((sPath == NULL) || (sPath[0] == '\0') || (sName == NULL) || (NameSize == 0u)) {
    return -1;
  }
  if ((strncmp(sPath, "Global\\", 7u) == 0) || (strncmp(sPath, "Local\\", 6u) == 0)) {
    return _SEGGER_SIM_RTT_CopyString(sName, NameSize, sPath);
  }
  if (_SEGGER_SIM_RTT_CopyString(sName, NameSize, "Local\\") != 0) {
    return -1;
  }
  DstOff = strlen(sName);
  SrcOff = (sPath[0] == '/') ? 1u : 0u;
  if (sPath[SrcOff] == '\0') {
    return -1;
  }
  while (sPath[SrcOff] != '\0') {
    if ((DstOff + 1u) >= NameSize) {
      return -1;
    }
    sName[DstOff++] = ((sPath[SrcOff] == '/') || (sPath[SrcOff] == '\\')) ? '_' : sPath[SrcOff];
    SrcOff++;
  }
  sName[DstOff] = '\0';
  return 0;
}

/*********************************************************************
*
*       _SEGGER_SIM_RTT_InitLock()
*
*  Function description
*    Initializes the host lock used by SEGGER_RTT_LOCK().
*/
static void _SEGGER_SIM_RTT_InitLock(void) {
  if (_IsLockInitialized == 0) {
    InitializeCriticalSection(&_Lock);
    _IsLockInitialized = 1;
  }
}

/*********************************************************************
*
*       _SEGGER_SIM_RTT_Map()
*
*  Function description
*    Opens or creates the Win32 file mapping used for RTT memory.
*
*  Parameters
*    sName  Mapping name.
*    Size   Required mapping size in bytes.
*
*  Return value
*    == 0  O.K.
*    != 0  Error.
*/
static int _SEGGER_SIM_RTT_Map(const char *sName, size_t Size) {
  DWORD Error;
  DWORD SizeHigh;
  DWORD SizeLow;

  SizeHigh = (DWORD)(((uint64_t)Size) >> 32);
  SizeLow  = (DWORD)(((uint64_t)Size) & 0xFFFFFFFFu);
  //
  // Open an existing mapping or create a new one.
  //
  _hMapping = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, SizeHigh, SizeLow, sName);
  if (_hMapping == NULL) {
    fprintf(stderr, "embOS MEMSHM: CreateFileMapping failed: %lu\n", GetLastError());
    return -1;
  }
  Error = GetLastError();
  _IsCreator = (Error != ERROR_ALREADY_EXISTS) ? 1 : 0;
  //
  // Map the object into the current process.
  //
  _pMappedBase = MapViewOfFile(_hMapping, FILE_MAP_ALL_ACCESS, 0u, 0u, Size);
  if (_pMappedBase == NULL) {
    fprintf(stderr, "embOS MEMSHM: MapViewOfFile failed: %lu\n", GetLastError());
    CloseHandle(_hMapping);
    _hMapping = NULL;
    _IsCreator = 0;
    return -1;
  }
  if (_IsCreator != 0) {
    memset(_pMappedBase, 0, Size);
  }
  return 0;
}

/*********************************************************************
*
*       Public code
*
**********************************************************************
*/

/*********************************************************************
*
*       SEGGER_SIM_RTT_EnsureMemory()
*
*  Function description
*    Ensures that the simulation RTT memory is mapped and initialized.
*
*  Return value
*    == 0  O.K.
*    != 0  Error.
*/
int SEGGER_SIM_RTT_EnsureMemory(void) {
  char   sName[256];
  size_t Size;

  if (_pMappedBase != NULL) {
    return 0;
  }
  //
  // Normalize the mapping name and map the object.
  //
  if (_SEGGER_SIM_RTT_NormalizeName(SEGGER_SIM_RTT_SHM_NAME, sName, sizeof(sName)) != 0) {
    fprintf(stderr, "embOS MEMSHM: invalid shared memory name: %s\n", SEGGER_SIM_RTT_SHM_NAME);
    return -1;
  }
  Size = SEGGER_SIM_RTT_REQUIRED_MEMORY_SIZE;
  if (_SEGGER_SIM_RTT_Map(sName, Size) != 0) {
    return -1;
  }
  _MappedSize = Size;
  _SEGGER_SIM_RTT_InitLock();
  //
  // Register cleanup and initialize the shared RTT layout.
  //
  if (_IsAtexitRegistered == 0) {
    if (atexit(SEGGER_SIM_RTT_CleanupMemory) != 0) {
      fprintf(stderr, "embOS MEMSHM: failed to register cleanup handler\n");
      SEGGER_SIM_RTT_CleanupMemory();
      return -1;
    }
    _IsAtexitRegistered = 1;
  }
  if (SEGGER_RTT_EnsureInitEx((PTR_ADDR)_pMappedBase, _MappedSize, SEGGER_SIM_RTT_NUM_CHANNELS) != 0) {
    fprintf(stderr, "embOS MEMSHM: failed to initialize RTT control block\n");
    SEGGER_SIM_RTT_CleanupMemory();
    return -1;
  }
  return 0;
}

/*********************************************************************
*
*       SEGGER_SIM_RTT_CleanupMemory()
*
*  Function description
*    Releases the local shared memory mapping.
*/
void SEGGER_SIM_RTT_CleanupMemory(void) {
  if (_pMappedBase != NULL) {
    UnmapViewOfFile(_pMappedBase);
    _pMappedBase = NULL;
  }
  if (_hMapping != NULL) {
    CloseHandle(_hMapping);
    _hMapping = NULL;
  }
  if (_IsLockInitialized != 0) {
    DeleteCriticalSection(&_Lock);
    _IsLockInitialized = 0;
  }
  _MappedSize = 0u;
  _IsCreator  = 0;
}

/*********************************************************************
*
*       SEGGER_SIM_RTT_Lock()
*
*  Function description
*    Locks RTT access for the simulation process.
*/
void SEGGER_SIM_RTT_Lock(void) {
  _SEGGER_SIM_RTT_InitLock();
  EnterCriticalSection(&_Lock);
}

/*********************************************************************
*
*       SEGGER_SIM_RTT_Unlock()
*
*  Function description
*    Unlocks RTT access for the simulation process.
*/
void SEGGER_SIM_RTT_Unlock(void) {
  LeaveCriticalSection(&_Lock);
}

/*********************************************************************
*
*       SEGGER_SIM_RTT_GetMemoryAddress()
*
*  Function description
*    Returns the local mapped RTT control block address.
*
*  Return value
*    != 0  Local mapped RTT control block address.
*    == 0  Error.
*/
PTR_ADDR SEGGER_SIM_RTT_GetMemoryAddress(void) {
  if (SEGGER_SIM_RTT_EnsureMemory() != 0) {
    return 0u;
  }
  return (PTR_ADDR)_pMappedBase;
}

/*********************************************************************
*
*       SEGGER_SIM_RTT_GetMemorySize()
*
*  Function description
*    Returns the mapped RTT memory size.
*
*  Return value
*    >  0  Mapped RTT memory size in bytes.
*    == 0  Error.
*/
size_t SEGGER_SIM_RTT_GetMemorySize(void) {
  if (SEGGER_SIM_RTT_EnsureMemory() != 0) {
    return 0u;
  }
  return _MappedSize;
}

/*********************************************************************
*
*       SEGGER_SIM_RTT_GetSystemViewUpBufferAddress()
*
*  Function description
*    Returns the local mapped SystemView RTT up-buffer address.
*
*  Return value
*    != 0  Local mapped up-buffer address.
*    == 0  Error.
*/
PTR_ADDR SEGGER_SIM_RTT_GetSystemViewUpBufferAddress(void) {
  if (SEGGER_SIM_RTT_EnsureMemory() != 0) {
    return 0u;
  }
  return (PTR_ADDR)_pMappedBase + SEGGER_SIM_SYSVIEW_UP_BUFFER_OFF;
}

/*********************************************************************
*
*       SEGGER_SIM_RTT_GetSystemViewDownBufferAddress()
*
*  Function description
*    Returns the local mapped SystemView RTT down-buffer address.
*
*  Return value
*    != 0  Local mapped down-buffer address.
*    == 0  Error.
*/
PTR_ADDR SEGGER_SIM_RTT_GetSystemViewDownBufferAddress(void) {
  if (SEGGER_SIM_RTT_EnsureMemory() != 0) {
    return 0u;
  }
  return (PTR_ADDR)_pMappedBase + SEGGER_SIM_SYSVIEW_DOWN_BUFFER_OFF;
}

/*************************** End of file ****************************/
