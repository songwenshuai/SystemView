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
File    : LogHexdump.c
Purpose : Hex dump logging
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

static const char hexchar[] = "0123456789ABCDEF";

/*********************************************************************
*
*       LOG_Hexdump()
*
*  Function description
*    Print hexadecimal dump of a memory buffer.
*    Displays data in hex format with optional ASCII representation.
*
*  Parameters
*    inbuf   Pointer to buffer to dump
*    inlen   Length of buffer in bytes
*    ascii   If true, display ASCII representation
*    addr    If true, display address offsets
*/
void LOG_Hexdump(void *inbuf, unsigned inlen, bool ascii, bool addr) {
  unsigned char *cp = (unsigned char *)inbuf;
  unsigned char *ap = (unsigned char *)inbuf;
  int len = inlen;
  int clen, alen;
  char outbuf[96];
  char *outp = &outbuf[0];
  int  line = 0;

  Log_Print("====================================HEX DUMP START\n");
  while (len > 0) {
    if (addr)
      outp += sprintf(outp, "[0x%08" PRIxPTR "] ", (uintptr_t)cp);

    clen = alen = TRACEHUB_MIN(HEX_BYTES_PER_LINE, len);

    // display data in hex
    for (int i = 0; i < HEX_BYTES_PER_LINE; i++) {
      if (--clen >= 0) {
        unsigned char uc = *cp++;
        *outp++ = hexchar[(uc >> 4) & 0x0f];
        *outp++ = hexchar[(uc) & 0x0f];
        *outp++ = ' ';
      }
      else if (line != 0) {
        *outp++ = ' ';
        *outp++ = ' ';
        *outp++ = ' ';
      }
    }

    if (ascii) {
      *outp++ = ' ';
      *outp++ = ' ';

      // display data in ascii
      while (--alen >= 0) {
        unsigned char uc = *ap++;
        *outp++ = ((uc >= 0x20) && (uc < 0x7f)) ? uc : '.';
      }
    }

    // output the line
    *outp++ = '\n';
    *outp++ = '\0';
    Log_Print("%s", outbuf);
    outp = &outbuf[0];
    len -= HEX_BYTES_PER_LINE;
    line++;
  }
  Log_Print("====================================HEX DUMP END\n\n");
}
