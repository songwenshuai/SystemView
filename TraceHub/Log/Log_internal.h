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
File    : Log_internal.h
Purpose : Internal logging utility contracts
---------------------------END-OF-HEADER------------------------------
*/

#ifndef TRACEHUB_LOG_INTERNAL_H
#define TRACEHUB_LOG_INTERNAL_H

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "Log.h"
#include "SYS.h"

/*********************************************************************
*
*       Defines
*
**********************************************************************
*/

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

/*********************************************************************
*
*       Types
*
**********************************************************************
*/

/*********************************************************************
*
*       LOG_TextCleanStateInternal_t
*
*  Description
*    Internal state machine values stored in LOG_TextCleanState_t::state.
*/
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
*       Shared state
*
**********************************************************************
*/

extern FILE      *_main_log_file;
extern SYS_Mutex  _log_mutex;

/*********************************************************************
*
*       Text sanitizer functions
*
**********************************************************************
*/

int _Log_WriteCleanTextRange(FILE *file, const char *data, size_t len, LOG_TextCleanState_t *state, char *last_out, bool *emitted_out, size_t *clean_len_out);

#endif /* TRACEHUB_LOG_INTERNAL_H */

/*************************** End of file ****************************/
