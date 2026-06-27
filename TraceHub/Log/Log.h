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
File    : Log.h
Purpose : Logging utilities and hex dump functions for RTT bridge
---------------------------END-OF-HEADER------------------------------
*/

#ifndef TRACEHUB_LOG_H            // Guard against multiple inclusion
#define TRACEHUB_LOG_H

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

#if defined(__cplusplus)         // Allow usage of this module from C++ files (disable name mangling)
extern "C" {     /* Make sure we have C-declarations in C++ programs */
#endif

/*********************************************************************
*
*       Defines
*
**********************************************************************
*/

#ifdef _LOG_DEBUG
    #define Log_Print(...) do { \
        LOG_Debug(__FILE__, __LINE__, __FUNCTION__, __VA_ARGS__); \
    } while(0)
#else
    #define Log_Print(...) do { \
        if (0) { \
            LOG_Debug(NULL, 0, NULL, __VA_ARGS__); \
        } \
    } while(0)
#endif

#define Log_Error(...)  LOG_Error(__VA_ARGS__)
#define Log_Warn(...)   LOG_Warn(__VA_ARGS__)

/*********************************************************************
*
*       Types
*
**********************************************************************
*/

/*********************************************************************
*
*       LOG_TextCleanState_t
*
*  Description
*    Stateful text log sanitizer context.
*/
typedef struct {
    unsigned state;
    unsigned utf8_expected;
    unsigned utf8_length;
    uint32_t utf8_codepoint;
    unsigned char utf8_bytes[4];
} LOG_TextCleanState_t;

/*********************************************************************
*
*       API functions
*
**********************************************************************
*/

int    LOG_Init                 (void);
int    LOG_InitEx               (const char *prefix);
void   LOG_Cleanup              (void);
FILE * LOG_GetMainFile          (void);
FILE * LOG_CreateTimestampedFile(const char *prefix);
FILE * LOG_CreateTimestampedFileEx(const char *prefix, const char *extension, const char *mode);
void   LOG_Hexdump              (void *inbuf, unsigned inlen, bool ascii, bool addr);
void   LOG_LogToFile            (FILE *file, const char *sFormat, ...);
void   LOG_Debug                (const char *file, int line, const char *function, const char *sFormat, ...);
void   LOG_Error                (const char *sFormat, ...);
void   LOG_Warn                 (const char *sFormat, ...);
void   LOG_TextCleanStateInit   (LOG_TextCleanState_t *state);
int    LOG_WriteCleanTextToFile (FILE *file, const char *data, size_t len, LOG_TextCleanState_t *state);
int    LOG_WriteCleanTextToFileEx (FILE *file, const char *data, size_t len, LOG_TextCleanState_t *state, size_t *clean_len);
int    LOG_SwimLaneLogToFile    (FILE *file, uint64_t timestamp_us, const char *source, const char *content, LOG_TextCleanState_t *state);

#if defined(__cplusplus)          // Allow usage of this module from C++ files (disable name mangling)
}                /* Make sure we have C-declarations in C++ programs */
#endif

#endif // end of include guard: TRACEHUB_LOG_H Avoid multiple inclusion


/*************************** End of file ****************************/
