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
File    : RTTSimCommon.h
Purpose : Shared helpers for RTT simulation programs
---------------------------END-OF-HEADER------------------------------
*/

#ifndef RTT_SIM_COMMON_H
#define RTT_SIM_COMMON_H

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include <stdint.h>
#include <stdio.h>

#include "SEGGER_RTT.h"

/*********************************************************************
*
*       Defines
*
**********************************************************************
*/

#define RTT_SIM_DOWN_BUFFER_SIZE         128u

/*********************************************************************
*
*       Static functions
*
**********************************************************************
*/

/*********************************************************************
*
*       RTT_SIM_PrintBytes()
*
*  Function description
*    Print raw bytes as hexadecimal values.
*/
static void RTT_SIM_PrintBytes(const unsigned char *data, unsigned num_bytes) {
    unsigned i;

    if (data == NULL || num_bytes == 0u) {
        return;
    }

    for (i = 0u; i < num_bytes; i++) {
        printf("%02X", data[i]);
        if ((i + 1u) < num_bytes) {
            printf(" ");
        }
    }
}

/*********************************************************************
*
*       RTT_SIM_DrainDownBuffer()
*
*  Function description
*    Read host-to-target bytes from a down-buffer and print them.
*/
static unsigned RTT_SIM_DrainDownBuffer(uintptr_t rtt_address,
                                        unsigned channel,
                                        const char *name) {
    unsigned char buffer[RTT_SIM_DOWN_BUFFER_SIZE];
    unsigned      bytes_read;

    bytes_read = SEGGER_RTT_ReadNoLock(rtt_address,
                                       channel,
                                       buffer,
                                       sizeof(buffer));
    if (bytes_read == 0u) {
        return 0u;
    }

    printf("Target RX [%s ch%u] %u bytes: ", name, channel, bytes_read);
    RTT_SIM_PrintBytes(buffer, bytes_read);
    printf("\n");

    return bytes_read;
}

#endif

/*************************** End of file ****************************/
