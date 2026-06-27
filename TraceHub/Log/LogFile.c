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
----------------------------------------------------------------------
File    : LogFile.c
Purpose : Timestamped log file creation
---------------------------END-OF-HEADER------------------------------
*/

#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "Log_internal.h"
#include "Utils.h"

#if defined(_WIN32)
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
  #endif
  #include <windows.h>
  #include <fcntl.h>
  #include <io.h>
  #include <process.h>
  #include <sys/stat.h>
#else
  #include <sys/time.h>
  #include <sys/types.h>
  #include <sys/stat.h>
  #include <fcntl.h>
  #include <unistd.h>
#endif

#ifndef PATH_MAX
  #define PATH_MAX 4096
#endif

#if defined(_WIN32)
  #define LOG_OPEN_WRONLY   _O_WRONLY
  #define LOG_OPEN_RDWR     _O_RDWR
  #define LOG_OPEN_APPEND   _O_APPEND
  #define LOG_OPEN_CREAT    _O_CREAT
  #define LOG_OPEN_EXCL     _O_EXCL
  #define LOG_OPEN_BINARY   _O_BINARY
  #define LOG_OPEN_MODE     (_S_IREAD | _S_IWRITE)
#else
  #define LOG_OPEN_WRONLY   O_WRONLY
  #define LOG_OPEN_RDWR     O_RDWR
  #define LOG_OPEN_APPEND   O_APPEND
  #define LOG_OPEN_CREAT    O_CREAT
  #define LOG_OPEN_EXCL     O_EXCL
  #define LOG_OPEN_BINARY   0
  #define LOG_OPEN_MODE     0666
#endif

/*********************************************************************
*
*       _Log_GetOpenFlags()
*
*  Function description
*    Convert the supported fopen creation modes to native open flags.
*/
static int _Log_GetOpenFlags(const char *mode, int *flags) {
  int open_flags;

  if ((mode == NULL) || (flags == NULL)) {
    return -1;
  }

  if ((strcmp(mode, "a") == 0) || (strcmp(mode, "ab") == 0)) {
    open_flags = LOG_OPEN_WRONLY | LOG_OPEN_APPEND;
  } else if ((strcmp(mode, "a+") == 0) || (strcmp(mode, "a+b") == 0) ||
             (strcmp(mode, "ab+") == 0)) {
    open_flags = LOG_OPEN_RDWR | LOG_OPEN_APPEND;
  } else if ((strcmp(mode, "w") == 0) || (strcmp(mode, "wb") == 0)) {
    open_flags = LOG_OPEN_WRONLY;
  } else if ((strcmp(mode, "w+") == 0) || (strcmp(mode, "w+b") == 0) ||
             (strcmp(mode, "wb+") == 0)) {
    open_flags = LOG_OPEN_RDWR;
  } else {
    return -1;
  }

  if (strchr(mode, 'b') != NULL) {
    open_flags |= LOG_OPEN_BINARY;
  }

  *flags = open_flags;
  return 0;
}


/*********************************************************************
*
*       _Log_OpenExclusive()
*
*  Function description
*    Create a new file descriptor and fail if the path already exists.
*/
static int _Log_OpenExclusive(const char *path, int open_flags) {
#if defined(_WIN32)
  return _open(path, open_flags | LOG_OPEN_CREAT | LOG_OPEN_EXCL, LOG_OPEN_MODE);
#else
  return open(path, open_flags | LOG_OPEN_CREAT | LOG_OPEN_EXCL, LOG_OPEN_MODE);
#endif
}


/*********************************************************************
*
*       _Log_FdOpen()
*
*  Function description
*    Convert a native file descriptor to a C stream.
*/
static FILE *_Log_FdOpen(int fd, const char *mode) {
#if defined(_WIN32)
  return _fdopen(fd, mode);
#else
  return fdopen(fd, mode);
#endif
}


/*********************************************************************
*
*       _Log_CloseDescriptor()
*
*  Function description
*    Close a native file descriptor.
*/
static void _Log_CloseDescriptor(int fd) {
#if defined(_WIN32)
  (void)_close(fd);
#else
  (void)close(fd);
#endif
}


/*********************************************************************
*
*       _Log_UnlinkPath()
*
*  Function description
*    Remove a path after stream creation failed.
*/
static void _Log_UnlinkPath(const char *path) {
#if defined(_WIN32)
  (void)_unlink(path);
#else
  (void)unlink(path);
#endif
}


/*********************************************************************
*
*       _Log_GetProcessId()
*
*  Function description
*    Return the current process ID for timestamped log file names.
*/
static long _Log_GetProcessId(void) {
#if defined(_WIN32)
  return (long)_getpid();
#else
  return (long)getpid();
#endif
}


/*********************************************************************
*
*       _Log_FormatCurrentTimestamp()
*
*  Function description
*    Format local wall-clock time and return its nanosecond field.
*/
static int _Log_FormatCurrentTimestamp(char *buffer,
                                       size_t buffer_size,
                                       long *nanoseconds) {
#if defined(_WIN32)
  FILETIME     utc_time;
  FILETIME     local_time;
  SYSTEMTIME   system_time;
  ULARGE_INTEGER ticks;
  int          length;

  if ((buffer == NULL) || (buffer_size == 0u) || (nanoseconds == NULL)) {
    return -1;
  }

  GetSystemTimeAsFileTime(&utc_time);
  ticks.LowPart = utc_time.dwLowDateTime;
  ticks.HighPart = utc_time.dwHighDateTime;
  *nanoseconds = (long)((ticks.QuadPart % 10000000ULL) * 100ULL);

  if (!FileTimeToLocalFileTime(&utc_time, &local_time) ||
      !FileTimeToSystemTime(&local_time, &system_time)) {
    return -1;
  }
  length = snprintf(buffer, buffer_size,
                    "%04u-%02u-%02uT%02u-%02u-%02u",
                    (unsigned)system_time.wYear,
                    (unsigned)system_time.wMonth,
                    (unsigned)system_time.wDay,
                    (unsigned)system_time.wHour,
                    (unsigned)system_time.wMinute,
                    (unsigned)system_time.wSecond);
  return ((length >= 0) && ((size_t)length < buffer_size)) ? 0 : -1;
#else
  struct tm       tm;
  struct timespec ts;
  struct timeval  tv;
  time_t          seconds;

  if ((buffer == NULL) || (buffer_size == 0u) || (nanoseconds == NULL)) {
    return -1;
  }

  if (clock_gettime(CLOCK_REALTIME, &ts) == 0) {
    seconds = (time_t)ts.tv_sec;
    *nanoseconds = ts.tv_nsec;
  } else {
    gettimeofday(&tv, NULL);
    seconds = (time_t)tv.tv_sec;
    *nanoseconds = (long)tv.tv_usec * 1000L;
  }
  if (localtime_r(&seconds, &tm) == NULL) {
    return -1;
  }
  return (strftime(buffer, buffer_size, "%Y-%m-%dT%H-%M-%S", &tm) == 0u) ? -1 : 0;
#endif
}


/*********************************************************************
*
*       LOG_CreateTimestampedFileEx()
*
*  Function description
*    Create a timestamped file with specified prefix, extension, and mode.
*    File name format:
*    <prefix>_YYYY-MM-DDTHH-MM-SS.nnnnnnnnn_pid<PID>_<seq>.<extension>
*
*  Parameters
*    prefix     Prefix for file name
*    extension  File extension without leading dot
*    mode       fopen mode
*
*  Return value
*    FILE*  Pointer to opened file
*    NULL   If file creation failed
*
*  Notes
*    (1) Timestamp is generated from system time at call moment
*    (2) The file is created with O_EXCL and never appends to an old run
*    (3) Caller must close the file when done
*/
FILE* LOG_CreateTimestampedFileEx(const char *prefix, const char *extension, const char *mode) {
  char            acTime[40];
  char            acFileName[PATH_MAX];
  FILE           *pFile = NULL;
  char           *sPath = NULL;
  long            nanoseconds;
  long            pid;
  unsigned        attempt;
  int             open_flags;
  int             fd;
  int             length;

  if ((prefix == NULL) || (extension == NULL) || (extension[0] == '\0') || (mode == NULL)) {
    return NULL;
  }
  if (_Log_GetOpenFlags(mode, &open_flags) != 0) {
    return NULL;
  }

  if (_Log_FormatCurrentTimestamp(acTime, sizeof(acTime), &nanoseconds) != 0) {
    return NULL;
  }
  pid = _Log_GetProcessId();

  for (attempt = 0u; attempt < 1000u; attempt++) {
    length = snprintf(acFileName, sizeof(acFileName),
                      "%s_%s.%09ld_pid%ld_%03u.%s",
                      prefix,
                      acTime,
                      nanoseconds,
                      (long)pid,
                      attempt,
                      extension);
    if ((length < 0) || ((size_t)length >= sizeof(acFileName))) {
      return NULL;
    }

    sPath = UTILS_LRealPath(acFileName);
    if (sPath == NULL) {
      return NULL;
    }

    fd = _Log_OpenExclusive(sPath, open_flags);
    if (fd >= 0) {
      pFile = _Log_FdOpen(fd, mode);
      if (pFile == NULL) {
        _Log_CloseDescriptor(fd);
        _Log_UnlinkPath(sPath);
      }
      free(sPath);
      return pFile;
    }

    if (errno != EEXIST) {
      free(sPath);
      return NULL;
    }
    free(sPath);
  }

  return NULL;
}


/*********************************************************************
*
*       LOG_CreateTimestampedFile()
*
*  Function description
*    Create a timestamped log file with specified prefix.
*    File name format: <prefix>_YYYY-MM-DDTHH-MM-SS.nnnnnnnnn_pid<PID>_<seq>.log
*/
FILE* LOG_CreateTimestampedFile(const char *prefix) {
  return LOG_CreateTimestampedFileEx(prefix, "log", "a+");
}
