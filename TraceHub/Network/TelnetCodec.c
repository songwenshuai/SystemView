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
File    : TelnetCodec.c
Purpose : Telnet option negotiation and client data filtering
---------------------------END-OF-HEADER------------------------------
*/

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include <stddef.h>

#include "TelnetCodec.h"
#include "Log.h"

/*********************************************************************
*
*       Defines
*
**********************************************************************
*/

#define TELNET_IAC      0xFF
#define TELNET_SE       0xF0
#define TELNET_SB       0xFA
#define TELNET_WILL     0xFB
#define TELNET_WONT     0xFC
#define TELNET_DO       0xFD
#define TELNET_DONT     0xFE

#define TELOPT_ECHO     0x01
#define TELOPT_SGA      0x03
#define TELOPT_NAWS     0x1F

/*********************************************************************
*
*       Static const data
*
**********************************************************************
*/

static const unsigned char _telnet_negotiation[] = {
    TELNET_IAC, TELNET_WILL, TELOPT_ECHO,
    TELNET_IAC, TELNET_WILL, TELOPT_SGA,
    TELNET_IAC, TELNET_DO,   TELOPT_SGA,
    TELNET_IAC, TELNET_WONT, TELOPT_NAWS,
};

/*********************************************************************
*
*       Static functions
*
**********************************************************************
*/

/*********************************************************************
*
*       _TelnetCodec_AppendOptionResponse()
*
*  Function description
*    Append a Telnet option negotiation response.
*/
static void _TelnetCodec_AppendOptionResponse(unsigned char command,
                                              unsigned char option,
                                              char *response,
                                              unsigned response_size,
                                              unsigned *response_len) {
    unsigned char reply;

    if ((response == NULL) || (response_len == NULL) ||
        ((*response_len + 3u) > response_size)) {
        return;
    }

    switch (command) {
    case TELNET_DO:
        reply = ((option == TELOPT_ECHO) || (option == TELOPT_SGA)) ? TELNET_WILL : TELNET_WONT;
        break;
    case TELNET_DONT:
        reply = TELNET_WONT;
        break;
    case TELNET_WILL:
        reply = (option == TELOPT_SGA) ? TELNET_DO : TELNET_DONT;
        break;
    case TELNET_WONT:
        reply = TELNET_DONT;
        break;
    default:
        return;
    }

    response[(*response_len)++] = (char)TELNET_IAC;
    response[(*response_len)++] = (char)reply;
    response[(*response_len)++] = (char)option;
}

/*********************************************************************
*
*       Public functions
*
**********************************************************************
*/

/*********************************************************************
*
*       TelnetCodec_Reset()
*
*  Function description
*    Reset a Telnet codec state object to the data state.
*
*  Parameters
*    state  Codec state to reset.
*/
void TelnetCodec_Reset(TelnetCodec_State_t *state) {
    if (state == NULL) {
        return;
    }

    state->State = TELNET_CODEC_STATE_DATA;
    state->Command = 0u;
}

/*********************************************************************
*
*       TelnetCodec_SendNegotiation()
*
*  Function description
*    Send the Telnet option negotiation sequence to a connected socket.
*
*  Parameters
*    hSock  Connected socket handle.
*
*  Return value
*    0   Negotiation sequence sent.
*   -1   Send failed.
*/
int TelnetCodec_SendNegotiation(SYS_SOCKET_HANDLE hSock) {
    int Result;

    Result = SYS_SOCKET_SendAll(hSock,
                                _telnet_negotiation,
                                sizeof(_telnet_negotiation),
                                TELNET_CODEC_RESPONSE_TIMEOUT_MS);
    if (Result != (int)sizeof(_telnet_negotiation)) {
        Log_Warn("Failed to send Telnet negotiation: %d\n", Result);
        return -1;
    }
    Log_Print("Telnet negotiation complete, sent %d bytes\n", Result);
    return 0;
}

/*********************************************************************
*
*       TelnetCodec_FilterClientData()
*
*  Function description
*    Filter Telnet command bytes from client data and generate any
*    required Telnet command responses.
*
*  Parameters
*    state          Codec state.
*    input          Client input bytes.
*    input_len      Number of input bytes.
*    output         Filtered data output buffer.
*    output_size    Output buffer size in bytes.
*    response       Telnet response output buffer.
*    response_size  Response buffer size in bytes.
*    response_len   Number of response bytes produced.
*
*  Return value
*    Number of filtered data bytes produced.
*/
unsigned TelnetCodec_FilterClientData(TelnetCodec_State_t *state,
                                      const char *input,
                                      unsigned input_len,
                                      char *output,
                                      unsigned output_size,
                                      char *response,
                                      unsigned response_size,
                                      unsigned *response_len) {
    unsigned i;
    unsigned output_len;

    if (response_len != NULL) {
        *response_len = 0u;
    }
    if ((state == NULL) || (input == NULL) || (output == NULL) ||
        (response_len == NULL)) {
        return 0u;
    }

    output_len = 0u;
    for (i = 0u; i < input_len; i++) {
        unsigned char ch;

        ch = (unsigned char)input[i];
        switch (state->State) {
        case TELNET_CODEC_STATE_DATA:
            if (ch == TELNET_IAC) {
                state->State = TELNET_CODEC_STATE_IAC;
            } else if (output_len < output_size) {
                output[output_len++] = (char)ch;
            }
            break;

        case TELNET_CODEC_STATE_IAC:
            if (ch == TELNET_IAC) {
                if (output_len < output_size) {
                    output[output_len++] = (char)TELNET_IAC;
                }
                state->State = TELNET_CODEC_STATE_DATA;
            } else if ((ch == TELNET_DO) || (ch == TELNET_DONT) ||
                       (ch == TELNET_WILL) || (ch == TELNET_WONT)) {
                state->Command = ch;
                state->State = TELNET_CODEC_STATE_OPTION;
            } else if (ch == TELNET_SB) {
                state->State = TELNET_CODEC_STATE_SUBNEG;
            } else {
                state->State = TELNET_CODEC_STATE_DATA;
            }
            break;

        case TELNET_CODEC_STATE_OPTION:
            _TelnetCodec_AppendOptionResponse(state->Command, ch,
                                              response, response_size, response_len);
            state->Command = 0u;
            state->State = TELNET_CODEC_STATE_DATA;
            break;

        case TELNET_CODEC_STATE_SUBNEG:
            if (ch == TELNET_IAC) {
                state->State = TELNET_CODEC_STATE_SUBNEG_IAC;
            }
            break;

        case TELNET_CODEC_STATE_SUBNEG_IAC:
            if (ch == TELNET_SE) {
                state->State = TELNET_CODEC_STATE_DATA;
            } else if (ch != TELNET_IAC) {
                state->State = TELNET_CODEC_STATE_SUBNEG;
            }
            break;
        }
    }

    return output_len;
}

/*************************** End of file ****************************/
