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
File    : SwimLaneText.c
Purpose : Swimlane ANSI and UTF-8 display-width handling
---------------------------END-OF-HEADER------------------------------
*/

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include <string.h>
#include <wchar.h>

#include "SwimLaneRenderer_internal.h"

/*********************************************************************
*
*       Static functions
*
**********************************************************************
*/

#if defined(_WIN32)
/*********************************************************************
*
*       _WideCharInRange()
*
*  Function description
*    Return whether a wide character value falls in an inclusive range.
*/
static bool _WideCharInRange(uint32_t value, uint32_t first, uint32_t last) {
    return value >= first && value <= last;
}

/*********************************************************************
*
*       _WideCharIsCombining()
*
*  Function description
*    Return whether a wide character has zero terminal column width.
*/
static bool _WideCharIsCombining(uint32_t value) {
    return _WideCharInRange(value, 0x0300u, 0x036Fu) ||
           _WideCharInRange(value, 0x0483u, 0x0489u) ||
           _WideCharInRange(value, 0x0591u, 0x05BDu) ||
           value == 0x05BFu ||
           _WideCharInRange(value, 0x05C1u, 0x05C2u) ||
           _WideCharInRange(value, 0x05C4u, 0x05C5u) ||
           value == 0x05C7u ||
           _WideCharInRange(value, 0x0610u, 0x061Au) ||
           _WideCharInRange(value, 0x064Bu, 0x065Fu) ||
           value == 0x0670u ||
           _WideCharInRange(value, 0x06D6u, 0x06DCu) ||
           _WideCharInRange(value, 0x06DFu, 0x06E4u) ||
           _WideCharInRange(value, 0x06E7u, 0x06E8u) ||
           _WideCharInRange(value, 0x06EAu, 0x06EDu) ||
           _WideCharInRange(value, 0x0711u, 0x0711u) ||
           _WideCharInRange(value, 0x0730u, 0x074Au) ||
           _WideCharInRange(value, 0x07A6u, 0x07B0u) ||
           _WideCharInRange(value, 0x07EBu, 0x07F3u) ||
           _WideCharInRange(value, 0x0816u, 0x0819u) ||
           _WideCharInRange(value, 0x081Bu, 0x0823u) ||
           _WideCharInRange(value, 0x0825u, 0x0827u) ||
           _WideCharInRange(value, 0x0829u, 0x082Du) ||
           _WideCharInRange(value, 0x0859u, 0x085Bu) ||
           _WideCharInRange(value, 0x08D3u, 0x08E1u) ||
           _WideCharInRange(value, 0x08E3u, 0x0902u) ||
           value == 0x093Au ||
           value == 0x093Cu ||
           _WideCharInRange(value, 0x0941u, 0x0948u) ||
           value == 0x094Du ||
           _WideCharInRange(value, 0x0951u, 0x0957u) ||
           _WideCharInRange(value, 0x0962u, 0x0963u) ||
           _WideCharInRange(value, 0x200Bu, 0x200Fu) ||
           _WideCharInRange(value, 0x202Au, 0x202Eu) ||
           _WideCharInRange(value, 0x2060u, 0x206Fu) ||
           _WideCharInRange(value, 0xFE00u, 0xFE0Fu) ||
           _WideCharInRange(value, 0xFE20u, 0xFE2Fu);
}

/*********************************************************************
*
*       _WideCharIsWide()
*
*  Function description
*    Return whether a wide character normally occupies two columns.
*/
static bool _WideCharIsWide(uint32_t value) {
    return _WideCharInRange(value, 0x1100u, 0x115Fu) ||
           _WideCharInRange(value, 0x2329u, 0x232Au) ||
           _WideCharInRange(value, 0x2E80u, 0xA4CFu) ||
           _WideCharInRange(value, 0xAC00u, 0xD7A3u) ||
           _WideCharInRange(value, 0xF900u, 0xFAFFu) ||
           _WideCharInRange(value, 0xFE10u, 0xFE19u) ||
           _WideCharInRange(value, 0xFE30u, 0xFE6Fu) ||
           _WideCharInRange(value, 0xFF00u, 0xFF60u) ||
           _WideCharInRange(value, 0xFFE0u, 0xFFE6u) ||
           _WideCharInRange(value, 0x20000u, 0x3FFFDu);
}
#endif

/*********************************************************************
*
*       _GetWideCharDisplayWidth()
*
*  Function description
*    Return terminal column width for one wide character.
*/
static int _GetWideCharDisplayWidth(wchar_t wc) {
#if defined(_WIN32)
    uint32_t value;

    value = (uint32_t)wc;
    if (value == 0u) {
        return 0;
    }
    if (value < 0x20u || _WideCharInRange(value, 0x7Fu, 0x9Fu)) {
        return -1;
    }
    if (_WideCharIsCombining(value)) {
        return 0;
    }
    if (_WideCharIsWide(value)) {
        return 2;
    }
    return 1;
#else
    return wcwidth(wc);
#endif
}

/*********************************************************************
*
*       Public internal functions
*
**********************************************************************
*/

/*********************************************************************
*
*       SwimLaneText_AnsiEscapeLength()
*
*  Function description
*    Return the byte length of one complete ANSI escape sequence.
*/
unsigned SwimLaneText_AnsiEscapeLength(const char *str) {
    unsigned i;

    if (str == NULL || str[0] != ANSI_ESC || str[1] == '\0') {
        return 0u;
    }

    if (str[1] == '[') {
        i = 2u;
        while (str[i] != '\0' && i < ANSI_MAX_SEQUENCE_LEN) {
            unsigned char c = (unsigned char)str[i];

            i++;
            if (c >= 0x40u && c <= 0x7Eu) {
                return i;
            }
        }
        return 0u;
    }

    if (str[1] == ']') {
        i = 2u;
        while (str[i] != '\0' && i < ANSI_MAX_SEQUENCE_LEN) {
            if (str[i] == ANSI_BEL) {
                return i + 1u;
            }
            if (str[i] == ANSI_ESC && str[i + 1u] == '\\') {
                return i + 2u;
            }
            i++;
        }
        return 0u;
    }

    if ((unsigned char)str[1] >= 0x40u && (unsigned char)str[1] <= 0x5Fu) {
        return 2u;
    }

    return 0u;
}

/*********************************************************************
*
*       SwimLaneText_WriteBytes()
*
*  Function description
*    Write a source byte range to the output stream. When color output is
*    disabled, ANSI escape sequences are omitted while all visible bytes are
*    preserved.
*/
void SwimLaneText_WriteBytes(FILE *stream, const char *src, unsigned src_bytes, bool color_enabled) {
    unsigned pos;

    if (stream == NULL || src == NULL || src_bytes == 0u) {
        return;
    }

    if (color_enabled) {
        (void)fwrite(src, 1u, src_bytes, stream);
        return;
    }

    pos = 0u;
    while (pos < src_bytes) {
        unsigned ansi_len;

        ansi_len = SwimLaneText_AnsiEscapeLength(src + pos);
        if (ansi_len > 0u && ansi_len <= src_bytes - pos) {
            pos += ansi_len;
            continue;
        }

        (void)fwrite(src + pos, 1u, 1u, stream);
        pos++;
    }
}

/*********************************************************************
*
*       SwimLaneText_Utf8BytesForWidth()
*
*  Function description
*    Calculate the number of bytes from a UTF-8 string that fit within
*    a specified display column width.
*
*  Parameters
*    str        UTF-8 encoded string
*    max_bytes  Maximum number of source bytes available
*    max_cols   Maximum display columns allowed
*    out_cols   Optional output: actual display columns consumed
*
*  Return value
*    Number of bytes that fit within max_cols display width
*/
unsigned SwimLaneText_Utf8BytesForWidth(const char *str, unsigned max_bytes,
                                        unsigned max_cols, unsigned *out_cols) {
    mbstate_t   state;
    unsigned    cols = 0;
    unsigned    bytes = 0;
    const char *p = str;
    //
    // Handle NULL or empty string
    //
    if (out_cols != NULL) {
        *out_cols = 0;
    }
    if (str == NULL || max_bytes == 0u || max_cols == 0u) {
        return 0;
    }
    //
    // Initialize multibyte conversion state
    //
    memset(&state, 0, sizeof(state));
    //
    // Process characters until we reach max_cols or end of string
    //
    while (bytes < max_bytes && cols < max_cols) {
        wchar_t  wc;
        size_t   n;
        int      w;
        unsigned ansi_len;
        //
        // Target-side ANSI sequences affect terminal state but have no visible width.
        //
        ansi_len = SwimLaneText_AnsiEscapeLength(p);
        if (ansi_len > 0u && ansi_len <= max_bytes - bytes) {
            p += ansi_len;
            bytes += ansi_len;
            continue;
        }
        //
        // Convert next multibyte character to wide character
        //
        n = mbrtowc(&wc, p, max_bytes - bytes, &state);
        if (n == (size_t)-1 || n == (size_t)-2) {
            //
            // Invalid or incomplete UTF-8 sequence: treat as single byte with width 1
            //
            memset(&state, 0, sizeof(state));
            if (cols + 1 > max_cols) {
                break;
            }
            p++;
            bytes++;
            cols++;
            continue;
        }
        if (n == 0) {
            //
            // Null character encountered
            //
            break;
        }
        if (n > max_bytes - bytes) {
            break;
        }
        //
        // Get display width of the character
        //
        w = _GetWideCharDisplayWidth(wc);
        if (w < 0) {
            //
            // Non-printable control character: treat as width 1
            //
            w = 1;
        }
        //
        // Check if this character fits
        //
        if (cols + (unsigned)w > max_cols) {
            //
            // Character does not fit. Consume it if nothing has been emitted yet
            // so wide characters cannot stall wrapping.
            //
            if (bytes == 0) {
                p += n;
                bytes += (unsigned)n;
                cols = max_cols;
            }
            break;
        }
        //
        // Character fits, advance
        //
        p += n;
        bytes += (unsigned)n;
        cols += (unsigned)w;
    }
    while (bytes < max_bytes) {
        unsigned ansi_len;

        ansi_len = SwimLaneText_AnsiEscapeLength(p);
        if (ansi_len == 0u || ansi_len > max_bytes - bytes) {
            break;
        }
        p += ansi_len;
        bytes += ansi_len;
    }
    //
    // Return actual columns consumed if requested
    //
    if (out_cols != NULL) {
        *out_cols = cols;
    }
    return bytes;
}

/*************************** End of file ****************************/
