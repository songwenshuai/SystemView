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
File    : Log.c
Purpose : Logging utilities and hex dump functions for RTT bridge
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
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

#include "Log.h"
#include "Utils.h"
#include "RTTMemory.h"

/*********************************************************************
*
*       Defines
*
**********************************************************************
*/

#define BEL    0x07
#define BS     0x08
#define TAB    0x09
#define LF     0x0A
#define VT     0x0B
#define FF     0x0C
#define CR     0x0D
#define SO     0x0E
#define SI     0x0F
#define CAN    0x18
#define SUB    0x1A
#define ESC    0x1B
#define SS2    0x8E
#define SS3    0x8F
#define DCS    0x90
#define CSI    0x9B
#define ST     0x9C
#define OSC    0x9D
#define PM     0x9E
#define APC    0x9F

//
// Hex dump configuration.
//
#define HEX_BYTES_PER_LINE  16

#ifndef PATH_MAX
  #define PATH_MAX 4096
#endif

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

static const char  hexchar[] = "0123456789ABCDEF";
static FILE       *_main_log_file = NULL;
static pthread_mutex_t _log_mutex = PTHREAD_MUTEX_INITIALIZER;

/*********************************************************************
*
*       Static functions
*
**********************************************************************
*/

static void _Log_OutputUnlocked(FILE *file, const char *msg);
static void _Log_Output(FILE *file, const char *msg);

/*********************************************************************
*
*       _Log_GetOpenFlags()
*
*  Function description
*    Convert the supported fopen creation modes to POSIX open flags.
*/
static int _Log_GetOpenFlags(const char *mode, int *flags) {
  if ((mode == NULL) || (flags == NULL)) {
    return -1;
  }

  if ((strcmp(mode, "a") == 0) || (strcmp(mode, "ab") == 0)) {
    *flags = O_WRONLY | O_APPEND;
    return 0;
  }
  if ((strcmp(mode, "a+") == 0) || (strcmp(mode, "a+b") == 0) ||
      (strcmp(mode, "ab+") == 0)) {
    *flags = O_RDWR | O_APPEND;
    return 0;
  }
  if ((strcmp(mode, "w") == 0) || (strcmp(mode, "wb") == 0)) {
    *flags = O_WRONLY;
    return 0;
  }
  if ((strcmp(mode, "w+") == 0) || (strcmp(mode, "w+b") == 0) ||
      (strcmp(mode, "wb+") == 0)) {
    *flags = O_RDWR;
    return 0;
  }

  return -1;
}

/*********************************************************************
*
*       VT_FilterChar()
*
*  Function description
*    Filter a single character through VT100/ANSI escape sequence state machine.
*    Determines if a character should be output or filtered based on current state.
*
*  Parameters
*    chr       Input character (0-255)
*    vt_state  Pointer to VT state machine state (maintained across calls)
*
*  Return value
*    >= 0  Character to output
*    < 0   Character should be filtered (not output)
*/
static int VT_FilterChar(int chr, VT_State_t *vt_state) {
  if (*vt_state == VT_STATE_DROP_ONE) {
    *vt_state = VT_STATE_NORMAL;
    return -1;
  }
  // Handle normal ANSI escape mechanism
  // (Note that this terminates DCS strings!)
  if (*vt_state == VT_STATE_ESC && chr >= 0x40 && chr <= 0x5F) {
    *vt_state = VT_STATE_NORMAL;
    chr += 0x40;
  }
  switch (chr) {
  case CAN:
  case SUB:
    *vt_state = VT_STATE_NORMAL;
    return -1;
  case ESC:
    *vt_state = VT_STATE_ESC;
    return -1;
  case CSI:
    *vt_state = VT_STATE_CSI;
    return -1;
  case DCS:
  case OSC:    // VT320 commands
  case PM:
  case APC:
    *vt_state = VT_STATE_DCS;
    return -1;
  default:
    if ((chr & 0x6F) < 0x20) { // Check controls
      switch (chr) {
      // VT oddity -- controls go through regardless of state.
      case BEL : return -1;
      case BS  : return -1;
      case TAB : return -1;
      case LF  : return chr;
      case VT  : return -1;
      case FF  : return -1;
      case CR  : return -1;
      }
      return -1;
    }
    switch (*vt_state) {
    case VT_STATE_NORMAL:
      return chr;
    case VT_STATE_ESC:
      *vt_state = VT_STATE_NORMAL;
      switch (chr) {
      case 'c': case '7': case '8':
      case '=': case '>': case '~':
      case 'n': case '\123': case 'o':
      case '|':
        break;
      case '#': case ' ': case '(':
      case ')': case '*': case '+':
        *vt_state = VT_STATE_DROP_ONE;
        break;
      }
      return -1;
    case VT_STATE_CSI:
    case VT_STATE_DCS:
      if (chr >= 0x40 && chr <= 0x7E) {
        if (*vt_state == VT_STATE_CSI) {
          *vt_state = VT_STATE_NORMAL;
        } else {
          *vt_state = VT_STATE_DCS_STRING;
        }
      }
      return -1;
    case VT_STATE_DCS_STRING:
      // Ignore the control payload until the terminator is observed.
      return -1;
    case VT_STATE_DROP_ONE:
      // Already handled at function start
      return -1;
    }
  }
  return -1;
}

/*********************************************************************
*
*       VT_FilterBuffer()
*
*  Function description
*    Filter VT100/ANSI escape sequences from input buffer to output buffer.
*    Pure data transformation without I/O operations.
*
*  Parameters
*    outBuf      Output buffer for filtered data
*    outBufSize  Size of output buffer
*    inBuf       Input buffer containing raw data
*    inLen       Length of input data
*    vt_state    Pointer to VT state machine state (maintained across calls)
*
*  Return value
*    Number of bytes written to outBuf
*/
static uint32_t VT_FilterBuffer(char *outBuf, uint32_t outBufSize,
                                const char *inBuf, uint32_t inLen,
                                VT_State_t *vt_state) {
  uint32_t outIdx = 0;
  int      chr    = 0;
  int      out    = 0;

  while (inLen-- && outIdx < outBufSize) {
    chr = *inBuf++ & 0xFF;
    out = VT_FilterChar(chr, vt_state);
    if (out >= 0) {
      outBuf[outIdx++] = (char)out;
    }
  }
  return outIdx;
}

/*********************************************************************
*
*       Public functions
*
**********************************************************************
*/

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

  pthread_mutex_lock(&_log_mutex);
  if (_main_log_file != NULL) {
    pthread_mutex_unlock(&_log_mutex);
    return -1;
  }

  _main_log_file = LOG_CreateTimestampedFile(prefix);
  if (_main_log_file == NULL) {
    pthread_mutex_unlock(&_log_mutex);
    return -2;
  }
  pthread_mutex_unlock(&_log_mutex);
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
  pthread_mutex_lock(&_log_mutex);
  if (_main_log_file != NULL) {
    fclose(_main_log_file);
    _main_log_file = NULL;
  }
  pthread_mutex_unlock(&_log_mutex);
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

  pthread_mutex_lock(&_log_mutex);
  file = _main_log_file;
  pthread_mutex_unlock(&_log_mutex);

  return file;
}

/*********************************************************************
*
*       LOG_Hexdump()
*
*  Function description
*    Print hexadecimal dump of a memory buffer.
*    Displays data in hex format with optional ASCII representation.
*
*  Parameters
*    inbuf   Pointer to buffer to dump
*    inlen   Length of buffer in bytes
*    ascii   If true, display ASCII representation
*    addr    If true, display address offsets
*/
void LOG_Hexdump(void *inbuf, unsigned inlen, bool ascii, bool addr) {
  unsigned char *cp = (unsigned char *)inbuf;
  unsigned char *ap = (unsigned char *)inbuf;
  int len = inlen;
  int clen, alen;
  char outbuf[96];
  char *outp = &outbuf[0];
  int  line = 0;

  Log_Print("====================================HEX DUMP START\n");
  while (len > 0) {
    if (addr)
      outp += sprintf(outp, "[0x%08" PRIxPTR "] ", (uintptr_t)cp);

    clen = alen = TRACEHUB_MIN(HEX_BYTES_PER_LINE, len);

    // display data in hex
    for (int i = 0; i < HEX_BYTES_PER_LINE; i++) {
      if (--clen >= 0) {
        unsigned char uc = *cp++;
        *outp++ = hexchar[(uc >> 4) & 0x0f];
        *outp++ = hexchar[(uc) & 0x0f];
        *outp++ = ' ';
      }
      else if (line != 0) {
        *outp++ = ' ';
        *outp++ = ' ';
        *outp++ = ' ';
      }
    }

    if (ascii) {
      *outp++ = ' ';
      *outp++ = ' ';

      // display data in ascii
      while (--alen >= 0) {
        unsigned char uc = *ap++;
        *outp++ = ((uc >= 0x20) && (uc < 0x7f)) ? uc : '.';
      }
    }

    // output the line
    *outp++ = '\n';
    *outp++ = '\0';
    Log_Print("%s", outbuf);
    outp = &outbuf[0];
    len -= HEX_BYTES_PER_LINE;
    line++;
  }
  Log_Print("====================================HEX DUMP END\n\n");
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
  struct tm       tm;
  struct timespec ts;
  struct timeval  tv;
  FILE           *pFile = NULL;
  char           *sPath = NULL;
  time_t          seconds;
  long            nanoseconds;
  pid_t           pid;
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

  if (clock_gettime(CLOCK_REALTIME, &ts) == 0) {
    seconds = (time_t)ts.tv_sec;
    nanoseconds = ts.tv_nsec;
  } else {
    gettimeofday(&tv, NULL);
    seconds = (time_t)tv.tv_sec;
    nanoseconds = (long)tv.tv_usec * 1000L;
  }
  if (localtime_r(&seconds, &tm) == NULL) {
    return NULL;
  }
  if (strftime(acTime, sizeof(acTime), "%Y-%m-%dT%H-%M-%S", &tm) == 0u) {
    return NULL;
  }
  pid = getpid();

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

    fd = open(sPath, open_flags | O_CREAT | O_EXCL, 0666);
    if (fd >= 0) {
      pFile = fdopen(fd, mode);
      if (pFile == NULL) {
        close(fd);
        unlink(sPath);
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
  pthread_mutex_lock(&_log_mutex);
  _Log_OutputUnlocked(file, msg);
  pthread_mutex_unlock(&_log_mutex);
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

  pthread_mutex_lock(&_log_mutex);
  if (file != NULL && function != NULL) {
    if (_main_log_file != NULL) {
      fprintf(_main_log_file, "%s:%d, %s(): ", file, line, function);
    }
    fprintf(stderr, "%s:%d, %s(): ", file, line, function);
  }
  _Log_OutputUnlocked(_main_log_file, ac);
  pthread_mutex_unlock(&_log_mutex);
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
  char    ac[256];
  char    output[288];

  va_start(ParamList, sFormat);
  (void)vsnprintf(ac, (int)sizeof(ac), sFormat, ParamList);
  va_end(ParamList);

  (void)snprintf(output, sizeof(output), "[ERROR] %s", ac);
  pthread_mutex_lock(&_log_mutex);
  _Log_OutputUnlocked(_main_log_file, output);
  pthread_mutex_unlock(&_log_mutex);
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
  char    ac[256];
  char    output[288];

  va_start(ParamList, sFormat);
  (void)vsnprintf(ac, (int)sizeof(ac), sFormat, ParamList);
  va_end(ParamList);

  (void)snprintf(output, sizeof(output), "[WARN] %s", ac);
  pthread_mutex_lock(&_log_mutex);
  _Log_OutputUnlocked(_main_log_file, output);
  pthread_mutex_unlock(&_log_mutex);
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

/*********************************************************************
*
*       LOG_TelnetLogToFile()
*
*  Function description
*    Log RTT/Telnet data to specified file with VT100/ANSI escape sequence filtering.
*    Strips terminal control sequences while preserving printable content.
*
*  Parameters
*    file      File handle to write to (or NULL to skip)
*    inBuf     Buffer containing data to log
*    inLen     Length of data in buffer
*    vt_state  Pointer to caller-maintained VT state (for thread safety)
*
*  Return value
*    0   Success or skipped because file is NULL
*   -1   Invalid state or file write failed
*
*  Notes
*    (1) Filters out VT100/ANSI escape sequences
*    (2) Preserves printable characters and line feeds
*    (3) Caller must maintain vt_state for proper escape sequence handling
*    (4) Caller must create and manage file handle
*    (5) Thread-safe: each caller should have its own vt_state
*/
int LOG_TelnetLogToFile(FILE *file, const char *inBuf, uint32_t inLen, VT_State_t *vt_state) {
  char     outBuf[512];
  uint32_t outLen = 0;
  size_t   written;

  if (file == NULL) {
    return 0;
  }
  if (inBuf == NULL || vt_state == NULL) {
    return -1;
  }

  // Process input in chunks that fit output buffer
  // VT filtering only removes data, so output <= input
  while (inLen > 0) {
    uint32_t chunkSize = (inLen > sizeof(outBuf)) ? sizeof(outBuf) : inLen;
    outLen = VT_FilterBuffer(outBuf, sizeof(outBuf), inBuf, chunkSize, vt_state);
    if (outLen > 0) {
      errno = 0;
      written = fwrite(outBuf, 1, outLen, file);
      if (written != outLen) {
        return -1;
      }
    }
    inBuf += chunkSize;
    inLen -= chunkSize;
  }
  errno = 0;
  if (fflush(file) != 0) {
    return -1;
  }

  return 0;
}

/*********************************************************************
*
*       LOG_SwimLaneLogToFile()
*
*  Function description
*    Log a swimlane entry to file with timestamp and source.
*    Format: [timestamp_us] [SOURCE] content
*
*  Parameters
*    file          File handle to write to (or NULL to skip)
*    timestamp_us  Microsecond timestamp
*    source        Source identifier string ("LINUX" or "RTOS")
*    content       Log content string
*
*  Return value
*    0   Success or skipped because file is NULL
*   -1   Invalid input or file write failed
*
*  Notes
*    (1) Appends newline if content doesn't end with one
*    (2) Flushes file after write for real-time logging
*    (3) Caller must create and manage file handle
*/
int LOG_SwimLaneLogToFile(FILE *file, uint64_t timestamp_us, const char *source, const char *content) {
  size_t len;

  if (file == NULL) {
    return 0;
  }
  if (source == NULL || content == NULL) {
    return -1;
  }

  // Write timestamp, source, and content
  errno = 0;
  if (fprintf(file, "[%" PRIu64 "] [%s] %s", timestamp_us, source, content) < 0) {
    return -1;
  }

  // Ensure line ends with newline
  len = strlen(content);
  if (len == 0 || content[len - 1] != '\n') {
    errno = 0;
    if (fprintf(file, "\n") < 0) {
      return -1;
    }
  }

  errno = 0;
  if (fflush(file) != 0) {
    return -1;
  }

  return 0;
}

/*************************** End of file ****************************/
