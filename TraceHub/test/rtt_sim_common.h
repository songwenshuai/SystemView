/*********************************************************************
*                                                                    *
*                Copyright (C) 2023 xrTest Inc.                      *
*                      All rights reserved                           *
*                                                                    *
**********************************************************************
----------------------------------------------------------------------
File    : rtt_sim_common.h
Purpose : Shared helpers for RTT simulation programs
Author  : songwenshuai <songwenshuai@gmail.com>
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
