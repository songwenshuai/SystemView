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
#include <errno.h>

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

#include "SYS.h"
#include "Log.h"
#include "Utils.h"
#include "RTTMemory.h"

/*********************************************************************
*
*       Defines
*
**********************************************************************
*/

//
// Hex dump configuration.
//
#define HEX_BYTES_PER_LINE  16
#define LOG_CLEAN_BUF_SIZE  512u
#define LOG_TEXT_FILTER_DROP      (-1)
#define LOG_TEXT_FILTER_ERROR     (-2)
#define LOG_TEXT_FILTER_REPROCESS (-3)
#define LOG_ASCII_ESC             0x1Bu
#define LOG_ASCII_BEL             0x07u
#define LOG_ASCII_CAN             0x18u
#define LOG_ASCII_SUB             0x1Au
#define LOG_UTF8_MAX_BYTES        4u
#define LOG_C1_START              0x80u
#define LOG_C1_END                0x9Fu
#define LOG_C1_SS2                0x8Eu
#define LOG_C1_SS3                0x8Fu
#define LOG_C1_DCS                0x90u
#define LOG_C1_CSI                0x9Bu
#define LOG_C1_ST                 0x9Cu
#define LOG_C1_OSC                0x9Du
#define LOG_C1_PM                 0x9Eu
#define LOG_C1_APC                0x9Fu

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

typedef enum {
  LOG_TEXT_CLEAN_STATE_NORMAL = 0,
  LOG_TEXT_CLEAN_STATE_ESC,
  LOG_TEXT_CLEAN_STATE_CSI,
  LOG_TEXT_CLEAN_STATE_STRING,
  LOG_TEXT_CLEAN_STATE_STRING_ESC,
  LOG_TEXT_CLEAN_STATE_ESC_SKIP_ONE
} LOG_TextCleanStateInternal_t;

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

static const char  hexchar[] = "0123456789ABCDEF";
static FILE       *_main_log_file = NULL;
static SYS_Mutex   _log_mutex = SYS_MUTEX_INITIALIZER;

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
*       _Log_WriteRaw()
*
*  Function description
*    Write an exact byte range to a file.
*/
static int _Log_WriteRaw(FILE *file, const char *data, size_t len) {
  size_t written;

  if (len == 0u) {
    return 0;
  }
  if ((file == NULL) || (data == NULL)) {
    return -1;
  }

  errno = 0;
  written = fwrite(data, 1u, len, file);
  return (written == len) ? 0 : -1;
}

/*********************************************************************
*
*       _Log_FilterC1Control()
*
*  Function description
*    Filter one C1 control byte through the text sanitizer state machine.
*/
static int _Log_FilterC1Control(unsigned char ch, LOG_TextCleanState_t *state) {
  if (state->state == LOG_TEXT_CLEAN_STATE_STRING ||
      state->state == LOG_TEXT_CLEAN_STATE_STRING_ESC) {
    state->state = (ch == LOG_C1_ST) ? LOG_TEXT_CLEAN_STATE_NORMAL
                                     : LOG_TEXT_CLEAN_STATE_STRING;
    return LOG_TEXT_FILTER_DROP;
  }

  state->state = LOG_TEXT_CLEAN_STATE_NORMAL;
  switch (ch) {
  case LOG_C1_CSI:
    state->state = LOG_TEXT_CLEAN_STATE_CSI;
    break;
  case LOG_C1_DCS:
  case LOG_C1_OSC:
  case LOG_C1_PM:
  case LOG_C1_APC:
    state->state = LOG_TEXT_CLEAN_STATE_STRING;
    break;
  case LOG_C1_SS2:
  case LOG_C1_SS3:
    state->state = LOG_TEXT_CLEAN_STATE_ESC_SKIP_ONE;
    break;
  default:
    break;
  }
  return LOG_TEXT_FILTER_DROP;
}

/*********************************************************************
*
*       _Log_ClearUtf8State()
*
*  Function description
*    Reset pending UTF-8 decoding state without changing terminal state.
*/
static void _Log_ClearUtf8State(LOG_TextCleanState_t *state) {
  if (state == NULL) {
    return;
  }

  state->utf8_expected = 0u;
  state->utf8_length = 0u;
  state->utf8_codepoint = 0u;
  memset(state->utf8_bytes, 0, sizeof(state->utf8_bytes));
}

/*********************************************************************
*
*       _Log_Utf8CodepointIsValid()
*
*  Function description
*    Validate a completed UTF-8 code point.
*/
static bool _Log_Utf8CodepointIsValid(uint32_t codepoint, unsigned length) {
  uint32_t min_codepoint;

  switch (length) {
  case 2u:
    min_codepoint = 0x80u;
    break;
  case 3u:
    min_codepoint = 0x800u;
    break;
  case 4u:
    min_codepoint = 0x10000u;
    break;
  default:
    return false;
  }

  if (codepoint < min_codepoint) {
    return false;
  }
  if (codepoint >= 0xD800u && codepoint <= 0xDFFFu) {
    return false;
  }
  if (codepoint > 0x10FFFFu) {
    return false;
  }
  return true;
}

/*********************************************************************
*
*       _Log_StartUtf8Sequence()
*
*  Function description
*    Start a UTF-8 sequence for a non-ASCII text byte.
*/
static int _Log_StartUtf8Sequence(unsigned char ch, LOG_TextCleanState_t *state) {
  if (state == NULL) {
    return LOG_TEXT_FILTER_ERROR;
  }

  if (ch >= 0xC2u && ch <= 0xDFu) {
    state->utf8_expected = 1u;
    state->utf8_length = 1u;
    state->utf8_codepoint = (uint32_t)(ch & 0x1Fu);
    state->utf8_bytes[0] = ch;
    return 0;
  }
  if (ch >= 0xE0u && ch <= 0xEFu) {
    state->utf8_expected = 2u;
    state->utf8_length = 1u;
    state->utf8_codepoint = (uint32_t)(ch & 0x0Fu);
    state->utf8_bytes[0] = ch;
    return 0;
  }
  if (ch >= 0xF0u && ch <= 0xF4u) {
    state->utf8_expected = 3u;
    state->utf8_length = 1u;
    state->utf8_codepoint = (uint32_t)(ch & 0x07u);
    state->utf8_bytes[0] = ch;
    return 0;
  }

  return LOG_TEXT_FILTER_DROP;
}

/*********************************************************************
*
*       _Log_FilterUtf8Continuation()
*
*  Function description
*    Consume one pending UTF-8 continuation byte.
*/
static int _Log_FilterUtf8Continuation(unsigned char ch,
                                        LOG_TextCleanState_t *state,
                                        char *out,
                                        size_t *out_len) {
  unsigned      length;
  unsigned char bytes[LOG_UTF8_MAX_BYTES];
  uint32_t      codepoint;

  if (state == NULL || out == NULL || out_len == NULL) {
    return LOG_TEXT_FILTER_ERROR;
  }

  *out_len = 0u;
  if (ch < 0x80u || ch > 0xBFu) {
    _Log_ClearUtf8State(state);
    return LOG_TEXT_FILTER_REPROCESS;
  }
  if (state->utf8_length >= LOG_UTF8_MAX_BYTES) {
    _Log_ClearUtf8State(state);
    return LOG_TEXT_FILTER_DROP;
  }

  state->utf8_bytes[state->utf8_length] = ch;
  state->utf8_length++;
  state->utf8_codepoint = (state->utf8_codepoint << 6) | (uint32_t)(ch & 0x3Fu);
  state->utf8_expected--;
  if (state->utf8_expected > 0u) {
    return 0;
  }

  length = state->utf8_length;
  codepoint = state->utf8_codepoint;
  memcpy(bytes, state->utf8_bytes, length);
  _Log_ClearUtf8State(state);

  if (!_Log_Utf8CodepointIsValid(codepoint, length)) {
    return LOG_TEXT_FILTER_DROP;
  }
  if (codepoint >= LOG_C1_START && codepoint <= LOG_C1_END) {
    return _Log_FilterC1Control((unsigned char)codepoint, state);
  }

  memcpy(out, bytes, length);
  *out_len = length;
  return 0;
}

/*********************************************************************
*
*       _Log_FilterCleanChar()
*
*  Function description
*    Filter one byte through the text sanitizer state machine.
*
*  Return value
*    >= 0  Character to output
*    -1    Character should be filtered
*    -2    Invalid state
*/
static int _Log_FilterCleanChar(int chr, LOG_TextCleanState_t *state) {
  unsigned char ch;

  if (state == NULL) {
    return LOG_TEXT_FILTER_ERROR;
  }

  ch = (unsigned char)chr;
  //
  // Complete fixed-length ESC designator sequences.
  //
  if (state->state == LOG_TEXT_CLEAN_STATE_ESC_SKIP_ONE) {
    state->state = LOG_TEXT_CLEAN_STATE_NORMAL;
    return LOG_TEXT_FILTER_DROP;
  }

  //
  // Handle C0 controls before printable-state dispatch.
  //
  switch (ch) {
  case LOG_ASCII_CAN:
  case LOG_ASCII_SUB:
    state->state = LOG_TEXT_CLEAN_STATE_NORMAL;
    return LOG_TEXT_FILTER_DROP;
  case LOG_ASCII_ESC:
    if (state->state == LOG_TEXT_CLEAN_STATE_STRING ||
        state->state == LOG_TEXT_CLEAN_STATE_STRING_ESC) {
      state->state = LOG_TEXT_CLEAN_STATE_STRING_ESC;
    } else {
      state->state = LOG_TEXT_CLEAN_STATE_ESC;
    }
    return LOG_TEXT_FILTER_DROP;
  case LOG_ASCII_BEL:
    if (state->state == LOG_TEXT_CLEAN_STATE_STRING ||
        state->state == LOG_TEXT_CLEAN_STATE_STRING_ESC) {
      state->state = LOG_TEXT_CLEAN_STATE_NORMAL;
    }
    return LOG_TEXT_FILTER_DROP;
  case '\n':
  case '\t':
    return (state->state == LOG_TEXT_CLEAN_STATE_NORMAL) ? ch : LOG_TEXT_FILTER_DROP;
  case '\b':
  case '\v':
  case '\f':
  case '\r':
  case 0x7F:
    return LOG_TEXT_FILTER_DROP;
  default:
    if (ch < 0x20u) {
      return LOG_TEXT_FILTER_DROP;
    }
    break;
  }

  if (ch >= LOG_C1_START && ch <= LOG_C1_END) {
    return _Log_FilterC1Control(ch, state);
  }

  switch (state->state) {
  case LOG_TEXT_CLEAN_STATE_NORMAL:
    return ch;

  case LOG_TEXT_CLEAN_STATE_ESC:
    //
    // Classify the ESC dispatch byte and keep the sequence payload out of
    // persistent text logs.
    //
    state->state = LOG_TEXT_CLEAN_STATE_NORMAL;
    switch (ch) {
    case '[':
      state->state = LOG_TEXT_CLEAN_STATE_CSI;
      break;
    case ']':
    case 'P':
    case '^':
    case '_':
      state->state = LOG_TEXT_CLEAN_STATE_STRING;
      break;
    case 'N':
    case 'O':
    case '#':
    case '%':
    case ' ':
    case '(':
    case ')':
    case '*':
    case '+':
    case '-':
    case '.':
    case '/':
      state->state = LOG_TEXT_CLEAN_STATE_ESC_SKIP_ONE;
      break;
    default:
      break;
    }
    return LOG_TEXT_FILTER_DROP;

  case LOG_TEXT_CLEAN_STATE_CSI:
    //
    // CSI sequences end at the final byte range 0x40..0x7E.
    //
    if (ch >= 0x40u && ch <= 0x7Eu) {
      state->state = LOG_TEXT_CLEAN_STATE_NORMAL;
    }
    return LOG_TEXT_FILTER_DROP;

  case LOG_TEXT_CLEAN_STATE_STRING:
    return LOG_TEXT_FILTER_DROP;

  case LOG_TEXT_CLEAN_STATE_STRING_ESC:
    //
    // OSC/DCS/PM/APC string controls terminate on the ST sequence ESC '\'.
    //
    state->state = (ch == '\\') ? LOG_TEXT_CLEAN_STATE_NORMAL : LOG_TEXT_CLEAN_STATE_STRING;
    return LOG_TEXT_FILTER_DROP;

  case LOG_TEXT_CLEAN_STATE_ESC_SKIP_ONE:
    state->state = LOG_TEXT_CLEAN_STATE_NORMAL;
    return LOG_TEXT_FILTER_DROP;

  default:
    return LOG_TEXT_FILTER_ERROR;
  }
}

/*********************************************************************
*
*       _Log_FilterCleanBytes()
*
*  Function description
*    Filter one input byte and emit zero or more clean text bytes.
*/
static int _Log_FilterCleanBytes(unsigned char ch,
                                 LOG_TextCleanState_t *state,
                                 char *out,
                                 size_t *out_len) {
  int filtered;
  int result;

  if (state == NULL || out == NULL || out_len == NULL) {
    return -1;
  }

  for (;;) {
    *out_len = 0u;

    if (state->utf8_expected > 0u) {
      result = _Log_FilterUtf8Continuation(ch, state, out, out_len);
      if (result == LOG_TEXT_FILTER_REPROCESS) {
        continue;
      }
      return (result == LOG_TEXT_FILTER_ERROR) ? -1 : 0;
    }

    if (state->state == LOG_TEXT_CLEAN_STATE_NORMAL && ch >= 0x80u) {
      if (ch <= LOG_C1_END) {
        filtered = _Log_FilterC1Control(ch, state);
        return (filtered == LOG_TEXT_FILTER_ERROR) ? -1 : 0;
      }
      result = _Log_StartUtf8Sequence(ch, state);
      return (result == LOG_TEXT_FILTER_ERROR) ? -1 : 0;
    }

    filtered = _Log_FilterCleanChar(ch, state);
    if (filtered == LOG_TEXT_FILTER_ERROR) {
      return -1;
    }
    if (filtered == LOG_TEXT_FILTER_DROP) {
      return 0;
    }

    out[0] = (char)filtered;
    *out_len = 1u;
    return 0;
  }
}

/*********************************************************************
*
*       _Log_WriteCleanTextRange()
*
*  Function description
*    Write text after removing terminal control sequences.
*/
static int _Log_WriteCleanTextRange(FILE *file,
                                    const char *data,
                                    size_t len,
                                    LOG_TextCleanState_t *state,
                                    char *last_out,
                                    bool *emitted_out,
                                    size_t *clean_len_out) {
  char   out_buf[LOG_CLEAN_BUF_SIZE];
  char   last;
  bool   emitted_any;
  size_t clean_total;
  size_t out_len;
  size_t pos;

  if (clean_len_out != NULL) {
    *clean_len_out = 0u;
  }
  if (file == NULL) {
    return 0;
  }
  if ((data == NULL && len > 0u) || state == NULL) {
    return -1;
  }

  last        = '\0';
  emitted_any = false;
  clean_total = 0u;
  out_len     = 0u;
  //
  // The caller-owned state preserves split escape sequences across writes.
  //
  for (pos = 0u; pos < len; pos++) {
    char   clean_bytes[LOG_UTF8_MAX_BYTES];
    size_t clean_len;
    size_t clean_pos;

    if (_Log_FilterCleanBytes((unsigned char)data[pos],
                              state,
                              clean_bytes,
                              &clean_len) != 0) {
      return -1;
    }

    for (clean_pos = 0u; clean_pos < clean_len; clean_pos++) {
      //
      // Track the last emitted byte so line-oriented writers can decide whether
      // an additional newline is required after filtering.
      //
      last             = clean_bytes[clean_pos];
      emitted_any      = true;
      out_buf[out_len] = clean_bytes[clean_pos];
      out_len++;
      clean_total++;
      if (out_len == sizeof(out_buf)) {
        if (_Log_WriteRaw(file, out_buf, out_len) != 0) {
          return -1;
        }
        out_len = 0u;
      }
    }
  }

  //
  // Write the remaining clean bytes without forcing a stream flush.
  //
  if (_Log_WriteRaw(file, out_buf, out_len) != 0) {
    return -1;
  }
  if (last_out != NULL) {
    *last_out = last;
  }
  if (emitted_out != NULL) {
    *emitted_out = emitted_any;
  }
  if (clean_len_out != NULL) {
    *clean_len_out = clean_total;
  }
  return 0;
}

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
  SYS_MutexLock(&_log_mutex);
  _Log_OutputUnlocked(_main_log_file, output);
  SYS_MutexUnlock(&_log_mutex);
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
  SYS_MutexLock(&_log_mutex);
  _Log_OutputUnlocked(_main_log_file, output);
  SYS_MutexUnlock(&_log_mutex);
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
*       LOG_TextCleanStateInit()
*
*  Function description
*    Initialize text sanitizer state.
*/
void LOG_TextCleanStateInit(LOG_TextCleanState_t *state) {
  if (state != NULL) {
    memset(state, 0, sizeof(*state));
    state->state = LOG_TEXT_CLEAN_STATE_NORMAL;
  }
}

/*********************************************************************
*
*       LOG_WriteCleanTextToFile()
*
*  Function description
*    Write text to a file after removing terminal control sequences.
*/
int LOG_WriteCleanTextToFile(FILE *file, const char *data, size_t len, LOG_TextCleanState_t *state) {
  return LOG_WriteCleanTextToFileEx(file, data, len, state, NULL);
}

/*********************************************************************
*
*       LOG_WriteCleanTextToFileEx()
*
*  Function description
*    Write text to a file after removing terminal control sequences.
*/
int LOG_WriteCleanTextToFileEx(FILE *file,
                               const char *data,
                               size_t len,
                               LOG_TextCleanState_t *state,
                               size_t *clean_len) {
  return _Log_WriteCleanTextRange(file, data, len, state, NULL, NULL, clean_len);
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
*    source        Source identifier string
*    content       Log content string
*    state         Text sanitizer state
*
*  Return value
*    0   Success or skipped because file is NULL
*   -1   Invalid input or file write failed
*
*  Notes
*    (1) Appends newline if cleaned content doesn't end with one
*    (2) Flushes file after write for real-time logging
*    (3) Caller must create and manage file handle
*/
int LOG_SwimLaneLogToFile(FILE *file,
                          uint64_t timestamp_us,
                          const char *source,
                          const char *content,
                          LOG_TextCleanState_t *state) {
  size_t content_len;
  char   last_clean;
  bool   emitted_clean;

  if (file == NULL) {
    return 0;
  }
  if (source == NULL || content == NULL || state == NULL) {
    return -1;
  }

  //
  // The swimlane prefix is TraceHub metadata; only target content is sanitized.
  //
  errno = 0;
  if (fprintf(file, "[%" PRIu64 "] [%s] ", timestamp_us, source) < 0) {
    return -1;
  }
  content_len = strlen(content);
  if (_Log_WriteCleanTextRange(file,
                               content,
                               content_len,
                               state,
                               &last_clean,
                               &emitted_clean,
                               NULL) != 0) {
    return -1;
  }

  //
  // Preserve one physical line per swimlane entry after text cleanup.
  //
  if (!emitted_clean || last_clean != '\n') {
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
