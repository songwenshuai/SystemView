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
File    : LogMain.c
Purpose : Main diagnostic log output
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

FILE      *_main_log_file = NULL;
SYS_Mutex  _log_mutex = SYS_MUTEX_INITIALIZER;

/*********************************************************************
*
*       LOG_InitEx()
*
*  Function description
*    Initialize the main log file.
*    Creates a timestamped log file named "<prefix>_YYYY-MM-DDTHH-MM-SS.log".
*
*  Return value
*    0   Success
*   -1   Already initialized
*   -2   Failed to create log file
*/
int LOG_InitEx(const char *prefix) {
  if (prefix == NULL) {
    return -1;
  }

  SYS_MutexLock(&_log_mutex);
  if (_main_log_file != NULL) {
    SYS_MutexUnlock(&_log_mutex);
    return -1;
  }

  _main_log_file = LOG_CreateTimestampedFile(prefix);
  if (_main_log_file == NULL) {
    SYS_MutexUnlock(&_log_mutex);
    return -2;
  }
  SYS_MutexUnlock(&_log_mutex);
  return 0;
}


/*********************************************************************
*
*       LOG_Init()
*
*  Function description
*    Initialize main log file with the default prefix.
*/
int LOG_Init(void) {
  return LOG_InitEx("main");
}


/*********************************************************************
*
*       LOG_Cleanup()
*
*  Function description
*    Close the main log file and cleanup resources.
*/
void LOG_Cleanup(void) {
  SYS_MutexLock(&_log_mutex);
  if (_main_log_file != NULL) {
    fclose(_main_log_file);
    _main_log_file = NULL;
  }
  SYS_MutexUnlock(&_log_mutex);
}


/*********************************************************************
*
*       LOG_GetMainFile()
*
*  Function description
*    Get the main log file handle.
*
*  Return value
*    FILE*  Main log file handle (may be NULL if not initialized)
*/
FILE* LOG_GetMainFile(void) {
  FILE *file;

  SYS_MutexLock(&_log_mutex);
  file = _main_log_file;
  SYS_MutexUnlock(&_log_mutex);

  return file;
}


/*********************************************************************
*
*       _Log_OutputUnlocked()
*
*  Function description
*    Core output function that writes message to file and stderr.
*    Caller must hold _log_mutex.
*
*  Parameters
*    file  File handle to write to (or NULL to skip file output)
*    msg   Message string to output
*/
static void _Log_OutputUnlocked(FILE *file, const char *msg) {
  if (file != NULL) {
    fprintf(file, "%s", msg);
    fflush(file);
  }
  fprintf(stderr, "%s", msg);
  fflush(stderr);
}


/*********************************************************************
*
*       _Log_Output()
*
*  Function description
*    Write one complete diagnostic message under the log mutex.
*
*  Parameters
*    file  File handle to write to (or NULL to skip file output)
*    msg   Message string to output
*/
static void _Log_Output(FILE *file, const char *msg) {
  SYS_MutexLock(&_log_mutex);
  _Log_OutputUnlocked(file, msg);
  SYS_MutexUnlock(&_log_mutex);
}


/*********************************************************************
*
*       LOG_Debug()
*
*  Function description
*    Output a debug trace message to main log and stderr.
*/
void LOG_Debug(const char *file, int line, const char *function, const char* sFormat, ...) {
  va_list ParamList;
  char    ac[256];

  va_start(ParamList, sFormat);
  (void)vsnprintf(ac, (int)sizeof(ac), sFormat, ParamList);
  va_end(ParamList);

  SYS_MutexLock(&_log_mutex);
  if (file != NULL && function != NULL) {
    if (_main_log_file != NULL) {
      fprintf(_main_log_file, "%s:%d, %s(): ", file, line, function);
    }
    fprintf(stderr, "%s:%d, %s(): ", file, line, function);
  }
  _Log_OutputUnlocked(_main_log_file, ac);
  SYS_MutexUnlock(&_log_mutex);
}


/*********************************************************************
*
*       _Log_OutputPrefixed()
*
*  Function description
*    Output one always-on diagnostic message with a fixed severity prefix.
*/
static void _Log_OutputPrefixed(const char *prefix, const char *sFormat, va_list ParamList) {
  va_list ParamListCopy;
  char    ac[256];
  char    output[288];

  va_copy(ParamListCopy, ParamList);
  (void)vsnprintf(ac, (int)sizeof(ac), sFormat, ParamListCopy);
  va_end(ParamListCopy);
  (void)snprintf(output, sizeof(output), "%s%s", prefix, ac);
  SYS_MutexLock(&_log_mutex);
  _Log_OutputUnlocked(_main_log_file, output);
  SYS_MutexUnlock(&_log_mutex);
}


/*********************************************************************
*
*       LOG_Error()
*
*  Function description
*    Output an always-on error diagnostic to main log and stderr.
*/
void LOG_Error(const char* sFormat, ...) {
  va_list ParamList;

  va_start(ParamList, sFormat);
  _Log_OutputPrefixed("[ERROR] ", sFormat, ParamList);
  va_end(ParamList);
}


/*********************************************************************
*
*       LOG_Warn()
*
*  Function description
*    Output an always-on warning diagnostic to main log and stderr.
*/
void LOG_Warn(const char* sFormat, ...) {
  va_list ParamList;

  va_start(ParamList, sFormat);
  _Log_OutputPrefixed("[WARN] ", sFormat, ParamList);
  va_end(ParamList);
}


/*********************************************************************
*
*       LOG_Info()
*
*  Function description
*    Output an always-on informational diagnostic to main log and stderr.
*/
void LOG_Info(const char* sFormat, ...) {
  va_list ParamList;

  va_start(ParamList, sFormat);
  _Log_OutputPrefixed("[INFO] ", sFormat, ParamList);
  va_end(ParamList);
}


/*********************************************************************
*
*       LOG_LogToFile()
*
*  Function description
*    Output a formatted log message to specified file and stderr.
*
*  Parameters
*    file     File handle to write to (or NULL to skip file output)
*    sFormat  Format string with placeholders (printf-style)
*    ...      Variable arguments for format string
*
*  Notes
*    (1) If file is NULL, only writes to stderr
*    (2) Does NOT create or manage file handle
*    (3) Caller is responsible for file lifecycle
*/
void LOG_LogToFile(FILE *file, const char* sFormat, ...) {
  va_list ParamList;
  char    ac[256];

  va_start(ParamList, sFormat);
  (void)vsnprintf(ac, (int)sizeof(ac), sFormat, ParamList);
  va_end(ParamList);

  _Log_Output(file, ac);
}
