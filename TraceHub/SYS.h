/*********************************************************************
*                                                                    *
*                Copyright (C) 2023 xrTest Inc.                      *
*                      All rights reserved                           *
*                                                                    *
**********************************************************************
----------------------------------------------------------------------
File    : SYS.h
Purpose : System abstraction layer for thread and timing functions
Author  : songwenshuai <songwenshuai@gmail.com>
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
#include <pthread.h>        // For pthread_mutex_t (SYS_Mutex)

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
*    Platform-specific implementation (pthread_t on Linux).
*/
typedef pthread_t SYS_Thread;

/*********************************************************************
*
*       SYS_Mutex
*
*  Description
*    Mutex object for thread synchronization.
*    Platform-specific implementation (pthread_mutex_t on Linux).
*/
typedef pthread_mutex_t SYS_Mutex;

/*********************************************************************
*
*       API functions
*
**********************************************************************
*/

int      SYS_createThread(void (*threadEntry)(void*), void* context, SYS_Thread *pRetTid);
void     SYS_destroyThread(SYS_Thread handle);
void     SYS_ExitThread(void *status);
void     SYS_WaitThreadTerm(SYS_Thread pRetTid);
unsigned SYS_GetTickCount(void);
uint64_t SYS_GetMonotonicTimeUs(void);
void     SYS_Sleep(unsigned ms);

int      SYS_MutexInit(SYS_Mutex *mutex);
int      SYS_MutexLock(SYS_Mutex *mutex);
int      SYS_MutexUnlock(SYS_Mutex *mutex);
int      SYS_MutexDestroy(SYS_Mutex *mutex);

void     SYS_GetTimestampStr(char *buf, size_t size);

#if defined(__cplusplus)          // Allow usage of this module from C++ files (disable name mangling)
}                /* Make sure we have C-declarations in C++ programs */
#endif

#endif // end of include guard: TRACEHUB_SYS_H Avoid multiple inclusion


/*************************** End of file ****************************/
