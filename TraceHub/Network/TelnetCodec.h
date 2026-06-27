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
File    : TelnetCodec.h
Purpose : Telnet option negotiation and client data filtering
---------------------------END-OF-HEADER------------------------------
*/

#ifndef TRACEHUB_TELNETCODEC_H
#define TRACEHUB_TELNETCODEC_H

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include "Socket.h"

#if defined(__cplusplus)
extern "C" {
#endif

/*********************************************************************
*
*       Defines
*
**********************************************************************
*/

#define TELNET_CODEC_RESPONSE_TIMEOUT_MS  1000

/*********************************************************************
*
*       Types
*
**********************************************************************
*/

typedef enum {
    TELNET_CODEC_STATE_DATA = 0,
    TELNET_CODEC_STATE_IAC,
    TELNET_CODEC_STATE_OPTION,
    TELNET_CODEC_STATE_SUBNEG,
    TELNET_CODEC_STATE_SUBNEG_IAC
} TelnetCodec_StateKind_t;

typedef struct {
    TelnetCodec_StateKind_t State;
    unsigned char           Command;
} TelnetCodec_State_t;

/*********************************************************************
*
*       API functions
*
**********************************************************************
*/

void     TelnetCodec_Reset           (TelnetCodec_State_t *state);
int      TelnetCodec_SendNegotiation (SYS_SOCKET_HANDLE hSock);
unsigned TelnetCodec_FilterClientData(TelnetCodec_State_t *state, const char *input, unsigned input_len, char *output, unsigned output_size, char *response, unsigned response_size, unsigned *response_len);

#if defined(__cplusplus)
}
#endif

#endif /* TRACEHUB_TELNETCODEC_H */

/*************************** End of file ****************************/
