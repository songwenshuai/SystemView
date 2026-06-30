/*********************************************************************
*                     SEGGER Microcontroller GmbH                    *
*                        The Embedded Experts                        *
**********************************************************************
*                                                                    *
*       (c) 1995 - 2026 SEGGER Microcontroller GmbH                  *
*                                                                    *
*       Internet: segger.com  Support: support_embos@segger.com      *
*                                                                    *
**********************************************************************
*                                                                    *
*       embOS-Classic * Real time operating system                   *
*                                                                    *
*       Please note:                                                 *
*                                                                    *
*       Knowledge of this file may under no circumstances            *
*       be used to write a similar product or a real-time            *
*       operating system for in-house use.                           *
*                                                                    *
*       Thank you for your fairness !                                *
*                                                                    *
**********************************************************************
*                                                                    *
*       OS version: V5.22.0.0                                        *
*                                                                    *
**********************************************************************

-------------------------- END-OF-HEADER -----------------------------
File    : OS_ThreadSafe.c
Purpose : Thread safety interposition functions.

Additional information:
  This module serializes selected C library calls made from simulated
  embOS task context. Allocator wrappers are enabled by the POSIX
  simulation CMake option EMBOS_SIM_ENABLE_ALLOCATOR_INTERPOSITION.
*/

#define _GNU_SOURCE

#include "RTOS.h"

#if !defined(EMBOS_SIM_HOST_POSIX) || (EMBOS_SIM_HOST_POSIX == 0)
  #error "OS_ThreadSafe.c is only supported by POSIX embOS simulation builds."
#endif

#include <dlfcn.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*********************************************************************
*
*       Types, local
*
**********************************************************************
*/

typedef int     OS_FUNC_PUTS(const char*);
typedef int     OS_FUNC_FPUTS(const char*, FILE*);
typedef ssize_t OS_FUNC_WRITE(int, const void*, size_t);
typedef size_t  OS_FUNC_FWRITE(const void* restrict, size_t, size_t, FILE* restrict);

/*********************************************************************
*
*       Configuration
*
**********************************************************************
*/
#ifndef   EMBOS_SIM_ENABLE_ALLOCATOR_INTERPOSITION
  #define EMBOS_SIM_ENABLE_ALLOCATOR_INTERPOSITION  (0)
#endif

/*********************************************************************
*
*       Prototypes
*
**********************************************************************
*/
#if (EMBOS_SIM_ENABLE_ALLOCATOR_INTERPOSITION != 0)
void* __real_malloc (size_t Size);
void  __real_free   (void* pAddress);
void* __real_calloc (size_t NumObjects, size_t Size);
void* __real_realloc(void* pAddress, size_t Size);
#endif

static void  _EnsureOriginalIOFunctions(void);
static void  _FailLookup               (const char* sFunction);
static void  _InitOriginalIOFunctions  (void);
static void* _LoadFunction             (const char* sFunction);

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

static pthread_once_t _IOFunctionOnce = PTHREAD_ONCE_INIT;
static OS_FUNC_PUTS*   _pfPuts;
static OS_FUNC_FPUTS*  _pfFputs;
static OS_FUNC_WRITE*  _pfWrite;
static OS_FUNC_FWRITE* _pfFwrite;

/*********************************************************************
*
*       Local functions
*
**********************************************************************
*/

/*********************************************************************
*
*       _FailLookup()
*/
static void _FailLookup(const char* sFunction) {
  const char* sError;
  static const char sPrefix[] = "[ERROR] Could not load run-time address of ";
  static const char sSuffix[] = "\n";
  static const char sSeparator[] = "(): ";

  if (_pfWrite != NULL) {
    sError = dlerror();
    (void)_pfWrite(STDERR_FILENO, sPrefix, sizeof(sPrefix) - 1u);
    (void)_pfWrite(STDERR_FILENO, sFunction, strlen(sFunction));
    if (sError != NULL) {
      (void)_pfWrite(STDERR_FILENO, sSeparator, sizeof(sSeparator) - 1u);
      (void)_pfWrite(STDERR_FILENO, sError, strlen(sError));
    }
    (void)_pfWrite(STDERR_FILENO, sSuffix, sizeof(sSuffix) - 1u);
  }
  exit(EXIT_FAILURE);
}

/*********************************************************************
*
*       _LoadFunction()
*/
static void* _LoadFunction(const char* sFunction) {
  void* pFunction;

  dlerror();
  pFunction = dlsym(RTLD_NEXT, sFunction);
  if (pFunction == NULL) {
    _FailLookup(sFunction);
  }
  return pFunction;
}

/*********************************************************************
*
*       _InitOriginalIOFunctions()
*/
static void _InitOriginalIOFunctions(void) {
  _pfWrite  = (OS_FUNC_WRITE*)_LoadFunction("write");
  _pfPuts   = (OS_FUNC_PUTS*)_LoadFunction("puts");
  _pfFputs  = (OS_FUNC_FPUTS*)_LoadFunction("fputs");
  _pfFwrite = (OS_FUNC_FWRITE*)_LoadFunction("fwrite");
}

/*********************************************************************
*
*       _EnsureOriginalIOFunctions()
*/
static void _EnsureOriginalIOFunctions(void) {
  int Error;

  Error = pthread_once(&_IOFunctionOnce, _InitOriginalIOFunctions);
  if (Error != 0) {
    exit(EXIT_FAILURE);
  }
}

/*********************************************************************
*
*       Global functions
*
**********************************************************************
*/

#if (EMBOS_SIM_ENABLE_ALLOCATOR_INTERPOSITION != 0)
/*********************************************************************
*
*       __wrap_malloc()
*/
void* __wrap_malloc(size_t Size) {
  void* pAddress;

  OS_ThreadSafe_Lock();
  pAddress = __real_malloc(Size);
  OS_ThreadSafe_Unlock();
  return pAddress;
}

/*********************************************************************
*
*       __wrap_free()
*/
void __wrap_free(void* pAddress) {
  OS_ThreadSafe_Lock();
  __real_free(pAddress);
  OS_ThreadSafe_Unlock();
}

/*********************************************************************
*
*       __wrap_calloc()
*/
void* __wrap_calloc(size_t NumObjects, size_t Size) {
  void* pAddress;

  OS_ThreadSafe_Lock();
  pAddress = __real_calloc(NumObjects, Size);
  OS_ThreadSafe_Unlock();
  return pAddress;
}

/*********************************************************************
*
*       __wrap_realloc()
*/
void* __wrap_realloc(void* pAddress, size_t Size) {
  void* pNewAddress;

  OS_ThreadSafe_Lock();
  pNewAddress = __real_realloc(pAddress, Size);
  OS_ThreadSafe_Unlock();
  return pNewAddress;
}
#endif

/*********************************************************************
*
*       printf()
*/
int printf(const char* restrict pFormat, ...) {
  va_list Args;
  int     Result;

  _EnsureOriginalIOFunctions();
  va_start(Args, pFormat);
  OS_ThreadSafe_Lock();
  Result = vprintf(pFormat, Args);
  OS_ThreadSafe_Unlock();
  va_end(Args);
  return Result;
}

/*********************************************************************
*
*       fprintf()
*/
int fprintf(FILE* restrict pStream, const char* restrict pFormat, ...) {
  va_list Args;
  int     Result;

  _EnsureOriginalIOFunctions();
  va_start(Args, pFormat);
  OS_ThreadSafe_Lock();
  Result = vfprintf(pStream, pFormat, Args);
  OS_ThreadSafe_Unlock();
  va_end(Args);
  return Result;
}

/*********************************************************************
*
*       puts()
*/
int puts(const char* pStr) {
  int Result;

  _EnsureOriginalIOFunctions();
  OS_ThreadSafe_Lock();
  Result = _pfPuts(pStr);
  OS_ThreadSafe_Unlock();
  return Result;
}

/*********************************************************************
*
*       fputs()
*/
int fputs(const char* restrict pStr, FILE* restrict pStream) {
  int Result;

  _EnsureOriginalIOFunctions();
  OS_ThreadSafe_Lock();
  Result = _pfFputs(pStr, pStream);
  OS_ThreadSafe_Unlock();
  return Result;
}

/*********************************************************************
*
*       write()
*/
ssize_t write(int FileDescriptor, const void* pBuffer, size_t Count) {
  ssize_t Result;

  _EnsureOriginalIOFunctions();
  OS_ThreadSafe_Lock();
  Result = _pfWrite(FileDescriptor, pBuffer, Count);
  OS_ThreadSafe_Unlock();
  return Result;
}

/*********************************************************************
*
*       fwrite()
*/
size_t fwrite(const void* restrict pData, size_t Size, size_t NumObjects, FILE* restrict pStream) {
  size_t Result;

  _EnsureOriginalIOFunctions();
  OS_ThreadSafe_Lock();
  Result = _pfFwrite(pData, Size, NumObjects, pStream);
  OS_ThreadSafe_Unlock();
  return Result;
}

/*************************** End of file ****************************/
