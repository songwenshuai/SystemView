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
File    : LogText.c
Purpose : Text log sanitizer
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
int _Log_WriteCleanTextRange(FILE *file,
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
