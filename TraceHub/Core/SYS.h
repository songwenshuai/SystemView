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
File    : SYS.h
Purpose : System abstraction layer for thread and timing functions
---------------------------END-OF-HEADER------------------------------
*/

#ifndef TRACEHUB_SYS_H            // Guard against multiple inclusion
#define TRACEHUB_SYS_H

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include <stdbool.h>
#include <stdint.h>         // Type definitions: uint8_t, uint16_t, uint32_t, int8_t, int16_t, int32_t
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>         // For va_list.
#include <stddef.h>         // for size_t

#if defined(_WIN32)
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
  #endif
  #include <windows.h>
#else
  #include <pthread.h>        // For pthread_mutex_t (SYS_Mutex)
#endif

#if defined(__cplusplus)         // Allow usage of this module from C++ files (disable name mangling)
extern "C" {     /* Make sure we have C-declarations in C++ programs */
#endif

/*********************************************************************
*
*       Types
*
**********************************************************************
*/

/*********************************************************************
*
*       SYS_Thread
*
*  Description
*    Handle for system thread objects.
*/
#if defined(_WIN32)
typedef HANDLE SYS_Thread;
#else
typedef pthread_t SYS_Thread;
#endif

/*********************************************************************
*
*       SYS_Mutex
*
*  Description
*    Mutex object for thread synchronization.
*/
#if defined(_WIN32)
typedef SRWLOCK SYS_Mutex;
  #define SYS_MUTEX_INITIALIZER SRWLOCK_INIT
#else
typedef pthread_mutex_t SYS_Mutex;
  #define SYS_MUTEX_INITIALIZER PTHREAD_MUTEX_INITIALIZER
#endif

/*********************************************************************
*
*       API functions
*
**********************************************************************
*/

int      SYS_createThread      (void (*threadEntry)(void *), void *context, SYS_Thread *pRetTid);
void     SYS_destroyThread     (SYS_Thread handle);
void     SYS_ExitThread        (void *status);
void     SYS_WaitThreadTerm    (SYS_Thread pRetTid);
unsigned SYS_GetTickCount      (void);
uint64_t SYS_GetMonotonicTimeUs(void);
void     SYS_Sleep             (unsigned ms);
int      SYS_MutexInit         (SYS_Mutex *mutex);
int      SYS_MutexLock         (SYS_Mutex *mutex);
int      SYS_MutexUnlock       (SYS_Mutex *mutex);
int      SYS_MutexDestroy      (SYS_Mutex *mutex);
void     SYS_GetTimestampStr   (char *buf, size_t size);

#if defined(__cplusplus)          // Allow usage of this module from C++ files (disable name mangling)
}                /* Make sure we have C-declarations in C++ programs */
#endif

#endif // end of include guard: TRACEHUB_SYS_H Avoid multiple inclusion


/*************************** End of file ****************************/
