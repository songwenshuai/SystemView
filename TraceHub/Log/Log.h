/*********************************************************************
*                                                                    *
*                Copyright (C) 2023 xrTest Inc.                      *
*                      All rights reserved                           *
*                                                                    *
**********************************************************************
----------------------------------------------------------------------
File    : Log.h
Purpose : Logging utilities and hex dump functions for RTT bridge
Author  : songwenshuai <songwenshuai@gmail.com>
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
*       VT_State_t
*
*  Description
*    VT100/ANSI escape sequence filter state machine.
*    Each caller should maintain their own state for thread safety.
*/
typedef enum {
    VT_STATE_NORMAL = 0,
    VT_STATE_ESC,
    VT_STATE_CSI,
    VT_STATE_DCS,
    VT_STATE_DCS_STRING,
    VT_STATE_DROP_ONE
} VT_State_t;

/*********************************************************************
*
*       API functions
*
**********************************************************************
*/

//
// Main log file management API
//
int   LOG_Init(void);
int   LOG_InitEx(const char *prefix);
void  LOG_Cleanup(void);
FILE* LOG_GetMainFile(void);

//
// Basic logging API
//
FILE* LOG_CreateTimestampedFile(const char *prefix);
FILE* LOG_CreateTimestampedFileEx(const char *prefix, const char *extension, const char *mode);
void  LOG_Hexdump(void *inbuf, unsigned inlen, bool ascii, bool addr);
void  LOG_LogToFile(FILE *file, const char* sFormat, ...);
void  LOG_Debug(const char *file, int line, const char *function, const char* sFormat, ...);
void  LOG_Error(const char* sFormat, ...);
void  LOG_Warn(const char* sFormat, ...);

//
// Specialized logging API (with thread-safe VT state)
//
int   LOG_TelnetLogToFile(FILE *file, const char *inBuf, uint32_t inLen, VT_State_t *vt_state);
int   LOG_SwimLaneLogToFile(FILE *file, uint64_t timestamp_us, const char *source, const char *content);

#if defined(__cplusplus)          // Allow usage of this module from C++ files (disable name mangling)
}                /* Make sure we have C-declarations in C++ programs */
#endif

#endif // end of include guard: TRACEHUB_LOG_H Avoid multiple inclusion


/*************************** End of file ****************************/
