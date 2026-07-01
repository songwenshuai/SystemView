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
File    : SEGGER_RTT_Memory_Posix.c
Purpose : POSIX MEMSHM RTT memory ownership for embOS simulation.
---------------------------END-OF-HEADER------------------------------
*/

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include "SEGGER_RTT_Memory_Int.h"

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

static int             _hShm = -1;
static pthread_mutex_t _Lock;
static pthread_once_t  _LockOnce = PTHREAD_ONCE_INIT;
static int             _LockInitStatus;
static void           *_pMappedBase;
static size_t          _MappedSize;
static int             _IsCreator;
static int             _IsAtexitRegistered;

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
*    Validates and copies the configured POSIX shared memory name.
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
  if ((sPath == NULL) || (sPath[0] != '/') || (sName == NULL) || (NameSize == 0u)) {
    return -1;
  }
  return _SEGGER_SIM_RTT_CopyString(sName, NameSize, sPath);
}

/*********************************************************************
*
*       _SEGGER_SIM_RTT_InitLock()
*
*  Function description
*    Initializes the host lock used by SEGGER_RTT_LOCK().
*/
static void _SEGGER_SIM_RTT_InitLock(void) {
  pthread_mutexattr_t Attr;
  int                 Status;

  _LockInitStatus = -1;
  Status = pthread_mutexattr_init(&Attr);
  if (Status != 0) {
    return;
  }
  Status = pthread_mutexattr_settype(&Attr, PTHREAD_MUTEX_RECURSIVE);
  if (Status == 0) {
    Status = pthread_mutex_init(&_Lock, &Attr);
  }
  (void)pthread_mutexattr_destroy(&Attr);
  if (Status == 0) {
    _LockInitStatus = 0;
  }
}

/*********************************************************************
*
*       _SEGGER_SIM_RTT_EnsureLock()
*
*  Function description
*    Ensures that the host RTT lock is initialized.
*/
static void _SEGGER_SIM_RTT_EnsureLock(void) {
  int Status;

  Status = pthread_once(&_LockOnce, _SEGGER_SIM_RTT_InitLock);
  if ((Status != 0) || (_LockInitStatus != 0)) {
    fprintf(stderr, "embOS MEMSHM: failed to initialize recursive RTT lock\n");
    abort();
  }
}

/*********************************************************************
*
*       _SEGGER_SIM_RTT_Map()
*
*  Function description
*    Opens or creates the POSIX shared memory object used for RTT memory.
*
*  Parameters
*    sName  Shared memory object name.
*    pSize  In: required size in bytes. Out: mapped object size in bytes.
*
*  Return value
*    == 0  O.K.
*    != 0  Error.
*/
static int _SEGGER_SIM_RTT_Map(const char *sName, size_t *pSize) {
  struct stat Stat;

  if (pSize == NULL) {
    return -1;
  }
  //
  // Open an existing shared memory object or create a new one.
  //
  _hShm = shm_open(sName, O_RDWR, 0666);
  if (_hShm == -1) {
    _hShm = shm_open(sName, O_RDWR | O_CREAT | O_EXCL, 0666);
    if (_hShm == -1) {
      fprintf(stderr, "embOS MEMSHM: shm_open failed: %s\n", strerror(errno));
      return -1;
    }
    _IsCreator = 1;
    if (ftruncate(_hShm, (off_t)*pSize) == -1) {
      fprintf(stderr, "embOS MEMSHM: ftruncate failed: %s\n", strerror(errno));
      close(_hShm);
      shm_unlink(sName);
      _hShm = -1;
      _IsCreator = 0;
      return -1;
    }
  } else {
    _IsCreator = 0;
  }
  if (fstat(_hShm, &Stat) == -1) {
    fprintf(stderr, "embOS MEMSHM: fstat failed: %s\n", strerror(errno));
    close(_hShm);
    if (_IsCreator != 0) {
      shm_unlink(sName);
    }
    _hShm = -1;
    _IsCreator = 0;
    return -1;
  }
  if ((Stat.st_size < 0) || ((size_t)Stat.st_size < *pSize)) {
    fprintf(stderr,
            "embOS MEMSHM: existing shared memory object is too small: %zu bytes required, %zu bytes found\n",
            *pSize,
            (Stat.st_size < 0) ? 0u : (size_t)Stat.st_size);
    close(_hShm);
    if (_IsCreator != 0) {
      shm_unlink(sName);
    }
    _hShm = -1;
    _IsCreator = 0;
    return -1;
  }
  *pSize = (size_t)Stat.st_size;
  //
  // Map the shared memory object into the current process.
  //
  _pMappedBase = mmap(NULL, *pSize, PROT_READ | PROT_WRITE, MAP_SHARED, _hShm, 0);
  if (_pMappedBase == MAP_FAILED) {
    fprintf(stderr, "embOS MEMSHM: mmap failed: %s\n", strerror(errno));
    close(_hShm);
    if (_IsCreator != 0) {
      shm_unlink(sName);
    }
    _hShm = -1;
    _pMappedBase = NULL;
    _IsCreator = 0;
    return -1;
  }
  if (_IsCreator != 0) {
    memset(_pMappedBase, 0, *pSize);
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
  // Normalize the shared memory name and map the object.
  //
  if (_SEGGER_SIM_RTT_NormalizeName(SEGGER_SIM_RTT_SHM_NAME, sName, sizeof(sName)) != 0) {
    fprintf(stderr, "embOS MEMSHM: invalid shared memory name: %s\n", SEGGER_SIM_RTT_SHM_NAME);
    return -1;
  }
  Size = SEGGER_SIM_RTT_REQUIRED_MEMORY_SIZE;
  if (_SEGGER_SIM_RTT_Map(sName, &Size) != 0) {
    return -1;
  }
  _MappedSize = Size;
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
*
*  Notes
*    The shared memory object is not unlinked during normal cleanup.
*/
void SEGGER_SIM_RTT_CleanupMemory(void) {
  if ((_pMappedBase != NULL) && (_MappedSize != 0u)) {
    munmap(_pMappedBase, _MappedSize);
    _pMappedBase = NULL;
  }
  if (_hShm >= 0) {
    close(_hShm);
    _hShm = -1;
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
  int Status;

  _SEGGER_SIM_RTT_EnsureLock();
  Status = pthread_mutex_lock(&_Lock);
  if (Status != 0) {
    fprintf(stderr, "embOS MEMSHM: failed to lock RTT mutex\n");
    abort();
  }
}

/*********************************************************************
*
*       SEGGER_SIM_RTT_Unlock()
*
*  Function description
*    Unlocks RTT access for the simulation process.
*/
void SEGGER_SIM_RTT_Unlock(void) {
  int Status;

  _SEGGER_SIM_RTT_EnsureLock();
  Status = pthread_mutex_unlock(&_Lock);
  if (Status != 0) {
    fprintf(stderr, "embOS MEMSHM: failed to unlock RTT mutex\n");
    abort();
  }
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
