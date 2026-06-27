/*********************************************************************
*                                                                    *
*                Copyright (C) 2023 xrTest Inc.                      *
*                      All rights reserved                           *
*                                                                    *
**********************************************************************
----------------------------------------------------------------------
File    : TelnetCodec.h
Purpose : Telnet option negotiation and client data filtering
Author  : songwenshuai <songwenshuai@gmail.com>
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

void     TelnetCodec_Reset(TelnetCodec_State_t *state);
int      TelnetCodec_SendNegotiation(SYS_SOCKET_HANDLE hSock);
unsigned TelnetCodec_FilterClientData(TelnetCodec_State_t *state,
                                      const char *input,
                                      unsigned input_len,
                                      char *output,
                                      unsigned output_size,
                                      char *response,
                                      unsigned response_size,
                                      unsigned *response_len);

#if defined(__cplusplus)
}
#endif

#endif /* TRACEHUB_TELNETCODEC_H */

/*************************** End of file ****************************/
