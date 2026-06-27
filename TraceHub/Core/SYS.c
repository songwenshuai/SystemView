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
File    : SYS.c
Purpose : System abstraction layer for thread and timing functions
---------------------------END-OF-HEADER------------------------------
*/

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <limits.h>
#include <string.h>
#include <stdbool.h>
#include <inttypes.h>
#include <time.h>

#if defined(_WIN32)
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
  #endif
  #include <windows.h>
  #include <process.h>
#else
  #include <pthread.h>
  #include <sys/time.h>
  #include <unistd.h>
  #include <poll.h>
#endif

#include "SYS.h"

/*********************************************************************
*
*       Types
*
**********************************************************************
*/

typedef struct {
  void (*threadEntry)(void *);
  void  *context;
} SYS_ThreadStartContext_t;

/*********************************************************************
*
*       Static functions
*
**********************************************************************
*/

static
#if defined(_WIN32)
unsigned __stdcall
#else
void *
#endif
_SYS_ThreadTrampoline(void *arg) {
  SYS_ThreadStartContext_t *thread_context;
  void (*threadEntry)(void *);
  void *context;

  thread_context = (SYS_ThreadStartContext_t *)arg;
  threadEntry = thread_context->threadEntry;
  context = thread_context->context;
  free(thread_context);

  threadEntry(context);
#if defined(_WIN32)
  return 0u;
#else
  return NULL;
#endif
}

/*********************************************************************
*
*       Public functions
*
**********************************************************************
*/

/*********************************************************************
*
*       SYS_createThread()
*
*  Function description
*    Create a new system thread with the specified entry function.
*
*  Parameters
*    threadEntry  Pointer to thread entry function.
*    context      Context parameter passed to thread entry function.
*    pRetTid      Pointer to receive thread handle.
*
*  Return value
*     1  Success
*    -1  Failed to create thread
*/
int SYS_createThread(void (*threadEntry)(void*), void* context, SYS_Thread *pRetTid) {
  SYS_ThreadStartContext_t *thread_context;
#if defined(_WIN32)
  uintptr_t                 handle;
#else
  pthread_t                 tid;
  int                       ret;
#endif

  if ((threadEntry == NULL) || (pRetTid == NULL)) {
      return -1;
  }

  thread_context = (SYS_ThreadStartContext_t *)malloc(sizeof(SYS_ThreadStartContext_t));
  if (thread_context == NULL) {
      return -1;
  }
  thread_context->threadEntry = threadEntry;
  thread_context->context = context;

#if defined(_WIN32)
  handle = _beginthreadex(NULL, 0u, _SYS_ThreadTrampoline, thread_context, 0u, NULL);
  if (handle == 0u) {
      free(thread_context);
      printf("SYS_createThread: _beginthreadex failed\n");
      return -1;
  }
  *pRetTid = (HANDLE)handle;
#else
  ret = pthread_create(&tid, NULL, _SYS_ThreadTrampoline, thread_context);
  if (ret != 0) {
      free(thread_context);
      printf("SYS_createThread: pthread_create error: %s\n", strerror(ret));
      return -1;
  }

  *pRetTid = tid;
#endif

  return 1;
}

/*********************************************************************
*
*       SYS_destroyThread()
*
*  Function description
*    Detach a thread, allowing system resources to be reclaimed
*    automatically when the thread terminates.
*
*  Parameters
*    handle  Thread handle to detach.
*
*  Return value
*    None
*/
void SYS_destroyThread(SYS_Thread handle) {
#if defined(_WIN32)
    if (handle != NULL) {
        CloseHandle(handle);
    }
#else
    pthread_detach(handle);
#endif
}

/*********************************************************************
*
*       SYS_ExitThread()
*
*  Function description
*    Terminate the calling thread and return a status value.
*
*  Parameters
*    status  Exit status value returned to joining thread.
*
*  Return value
*    None (function does not return)
*/
void SYS_ExitThread(void *status) {
#if defined(_WIN32)
    _endthreadex((unsigned)(uintptr_t)status);
#else
    pthread_exit(status);
#endif
}

/*********************************************************************
*
*       SYS_WaitThreadTerm()
*
*  Function description
*    Wait for the specified thread to terminate.
*    Blocks until the thread exits.
*
*  Parameters
*    pRetTid  Thread handle to wait for.
*
*  Return value
*    None
*/
void SYS_WaitThreadTerm(SYS_Thread pRetTid) {
#if defined(_WIN32)
  if (pRetTid != NULL) {
    WaitForSingleObject(pRetTid, INFINITE);
    CloseHandle(pRetTid);
  }
#else
  pthread_join(pRetTid, NULL);
#endif
}

/*********************************************************************
*
*       SYS_GetTickCount()
*
*  Function description
*    Get system tick count in milliseconds since system boot.
*    Uses monotonic clock for consistent timing.
*
*  Return value
*    Tick count in milliseconds
*/
unsigned SYS_GetTickCount(void) {
#if defined(_WIN32)
  return (unsigned)GetTickCount64();
#else
  struct timespec Time;

  clock_gettime(CLOCK_MONOTONIC, &Time);
  return ((unsigned)Time.tv_sec * 1000) + ((unsigned)Time.tv_nsec / 1000000);
#endif
}

/*********************************************************************
*
*       SYS_GetMonotonicTimeUs()
*
*  Function description
*    Get monotonic elapsed time in microseconds.
*
*  Return value
*    Monotonic elapsed time in microseconds
*/
uint64_t SYS_GetMonotonicTimeUs(void) {
#if defined(_WIN32)
  LARGE_INTEGER frequency;
  LARGE_INTEGER counter;
  uint64_t      seconds;
  uint64_t      remainder;

  QueryPerformanceFrequency(&frequency);
  QueryPerformanceCounter(&counter);
  seconds   = (uint64_t)counter.QuadPart / (uint64_t)frequency.QuadPart;
  remainder = (uint64_t)counter.QuadPart % (uint64_t)frequency.QuadPart;
  return (seconds * 1000000u) + ((remainder * 1000000u) / (uint64_t)frequency.QuadPart);
#else
  struct timespec Time;

  clock_gettime(CLOCK_MONOTONIC, &Time);
  return ((uint64_t)Time.tv_sec * 1000000u) + ((uint64_t)Time.tv_nsec / 1000u);
#endif
}

/*********************************************************************
*
*       SYS_Sleep()
*
*  Function description
*    Suspends the execution of the program for the specified number of
*    milliseconds. It uses the `poll` function to introduce a delay.
*
*  Parameters
*    ms - The number of milliseconds to sleep.
*
*  Return value
*    None.
*/
void SYS_Sleep(unsigned ms) {
#if defined(_WIN32)
  Sleep(ms);
#else
  // Use the `poll` function to introduce a delay
  poll(NULL, 0, ms);
#endif
}

/*********************************************************************
*
*       SYS_MutexInit()
*
*  Function description
*    Initialize a mutex.
*
*  Parameters
*    mutex  Pointer to mutex to initialize.
*
*  Return value
*    0   Success
*   -1   Failed to initialize mutex
*/
int SYS_MutexInit(SYS_Mutex *mutex) {
    if (mutex == NULL) {
        return -1;
    }
#if defined(_WIN32)
    InitializeSRWLock(mutex);
    return 0;
#else
    return pthread_mutex_init(mutex, NULL);
#endif
}

/*********************************************************************
*
*       SYS_MutexLock()
*
*  Function description
*    Lock a mutex.
*
*  Parameters
*    mutex  Pointer to mutex to lock.
*
*  Return value
*    0   Success
*   -1   Failed to lock mutex
*/
int SYS_MutexLock(SYS_Mutex *mutex) {
    if (mutex == NULL) {
        return -1;
    }
#if defined(_WIN32)
    AcquireSRWLockExclusive(mutex);
    return 0;
#else
    return pthread_mutex_lock(mutex);
#endif
}

/*********************************************************************
*
*       SYS_MutexUnlock()
*
*  Function description
*    Unlock a mutex.
*
*  Parameters
*    mutex  Pointer to mutex to unlock.
*
*  Return value
*    0   Success
*   -1   Failed to unlock mutex
*/
int SYS_MutexUnlock(SYS_Mutex *mutex) {
    if (mutex == NULL) {
        return -1;
    }
#if defined(_WIN32)
    ReleaseSRWLockExclusive(mutex);
    return 0;
#else
    return pthread_mutex_unlock(mutex);
#endif
}

/*********************************************************************
*
*       SYS_MutexDestroy()
*
*  Function description
*    Destroy a mutex.
*
*  Parameters
*    mutex  Pointer to mutex to destroy.
*
*  Return value
*    0   Success
*   -1   Failed to destroy mutex
*/
int SYS_MutexDestroy(SYS_Mutex *mutex) {
    if (mutex == NULL) {
        return -1;
    }
#if defined(_WIN32)
    return 0;
#else
    return pthread_mutex_destroy(mutex);
#endif
}

/*********************************************************************
*
*       SYS_GetTimestampStr()
*
*  Function description
*    Get current time as formatted timestamp string.
*    Format: "HH:MM:SS.mmm" (hours:minutes:seconds.milliseconds)
*
*  Parameters
*    buf   Buffer to receive timestamp string
*    size  Size of buffer in bytes
*/
void SYS_GetTimestampStr(char *buf, size_t size) {
#if defined(_WIN32)
    SYSTEMTIME st;
#else
    struct timespec ts;
    struct tm       tm_info;
#endif

    if (buf == NULL || size == 0) {
        return;
    }

#if defined(_WIN32)
    GetLocalTime(&st);
    snprintf(buf, size, "%02u:%02u:%02u.%03u",
             (unsigned)st.wHour, (unsigned)st.wMinute,
             (unsigned)st.wSecond, (unsigned)st.wMilliseconds);
#else
    clock_gettime(CLOCK_REALTIME, &ts);
    localtime_r(&ts.tv_sec, &tm_info);

    snprintf(buf, size, "%02d:%02d:%02d.%03ld",
             tm_info.tm_hour, tm_info.tm_min, tm_info.tm_sec,
             ts.tv_nsec / 1000000);
#endif
}

/*************************** End of file ****************************/
