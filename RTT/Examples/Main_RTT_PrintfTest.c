/*********************************************************************
*                   (c) SEGGER Microcontroller GmbH                  *
*                        The Embedded Experts                        *
*                           www.segger.com                           *
**********************************************************************
*                                                                    *
*        SEGGER RTT * Real Time Transfer for embedded targets        *
*                  https://github.com/SEGGERMicro/RTT                *
*                                                                    *
**********************************************************************

---------------------------END-OF-HEADER------------------------------
Purpose : Sample application to demonstrate RTT bi-directional functionality

----------------------------------------------------------------------
*/

#define MAIN_C

#include <stdio.h>

#include "SEGGER_RTT.h"

volatile int _Cnt;

static unsigned char _aRTTMemory[SEGGER_RTT__REQUIRED_MEM_SIZE + SEGGER_RTT__CB_ALIGNMENT];

/*********************************************************************
*
*       _GetRTTAddress
*/
static PTR_ADDR _GetRTTAddress(void) {
  return ((PTR_ADDR)_aRTTMemory + SEGGER_RTT__CB_ALIGNMENT_MASK) & ~(PTR_ADDR)SEGGER_RTT__CB_ALIGNMENT_MASK;
}

/*********************************************************************
*
*       main
*/
void main(void) {
  PTR_ADDR RTTAddress;

  RTTAddress = _GetRTTAddress();
  SEGGER_RTT_Init(RTTAddress);
  SEGGER_RTT_ConfigUpBuffer(RTTAddress, 0u, NULL, NULL, 0, SEGGER_RTT_MODE_BLOCK_IF_FIFO_FULL);

  SEGGER_RTT_WriteString(RTTAddress, 0u, "SEGGER Real-Time-Terminal Sample\r\n\r\n");
  SEGGER_RTT_WriteString(RTTAddress, 0u, "###### Testing SEGGER_printf() ######\r\n");

  SEGGER_RTT_printf(RTTAddress, 0u, "printf Test: %%c,         'S' : %c.\r\n", 'S');
  SEGGER_RTT_printf(RTTAddress, 0u, "printf Test: %%5c,        'E' : %5c.\r\n", 'E');
  SEGGER_RTT_printf(RTTAddress, 0u, "printf Test: %%-5c,       'G' : %-5c.\r\n", 'G');
  SEGGER_RTT_printf(RTTAddress, 0u, "printf Test: %%5.3c,      'G' : %-5c.\r\n", 'G');
  SEGGER_RTT_printf(RTTAddress, 0u, "printf Test: %%.3c,       'E' : %-5c.\r\n", 'E');
  SEGGER_RTT_printf(RTTAddress, 0u, "printf Test: %%c,         'R' : %c.\r\n", 'R');

  SEGGER_RTT_printf(RTTAddress, 0u, "printf Test: %%s,      \"RTT\" : %s.\r\n", "RTT");
  SEGGER_RTT_printf(RTTAddress, 0u, "printf Test: %%s, \"RTT\\r\\nRocks.\" : %s.\r\n", "RTT\r\nRocks.");

  SEGGER_RTT_printf(RTTAddress, 0u, "printf Test: %%u,       12345 : %u.\r\n", 12345);
  SEGGER_RTT_printf(RTTAddress, 0u, "printf Test: %%+u,      12345 : %+u.\r\n", 12345);
  SEGGER_RTT_printf(RTTAddress, 0u, "printf Test: %%.3u,     12345 : %.3u.\r\n", 12345);
  SEGGER_RTT_printf(RTTAddress, 0u, "printf Test: %%.6u,     12345 : %.6u.\r\n", 12345);
  SEGGER_RTT_printf(RTTAddress, 0u, "printf Test: %%6.3u,    12345 : %6.3u.\r\n", 12345);
  SEGGER_RTT_printf(RTTAddress, 0u, "printf Test: %%8.6u,    12345 : %8.6u.\r\n", 12345);
  SEGGER_RTT_printf(RTTAddress, 0u, "printf Test: %%08u,     12345 : %08u.\r\n", 12345);
  SEGGER_RTT_printf(RTTAddress, 0u, "printf Test: %%08.6u,   12345 : %08.6u.\r\n", 12345);
  SEGGER_RTT_printf(RTTAddress, 0u, "printf Test: %%0u,      12345 : %0u.\r\n", 12345);
  SEGGER_RTT_printf(RTTAddress, 0u, "printf Test: %%-.6u,    12345 : %-.6u.\r\n", 12345);
  SEGGER_RTT_printf(RTTAddress, 0u, "printf Test: %%-6.3u,   12345 : %-6.3u.\r\n", 12345);
  SEGGER_RTT_printf(RTTAddress, 0u, "printf Test: %%-8.6u,   12345 : %-8.6u.\r\n", 12345);
  SEGGER_RTT_printf(RTTAddress, 0u, "printf Test: %%-08u,    12345 : %-08u.\r\n", 12345);
  SEGGER_RTT_printf(RTTAddress, 0u, "printf Test: %%-08.6u,  12345 : %-08.6u.\r\n", 12345);
  SEGGER_RTT_printf(RTTAddress, 0u, "printf Test: %%-0u,     12345 : %-0u.\r\n", 12345);

  SEGGER_RTT_printf(RTTAddress, 0u, "printf Test: %%u,      -12345 : %u.\r\n", -12345);
  SEGGER_RTT_printf(RTTAddress, 0u, "printf Test: %%+u,     -12345 : %+u.\r\n", -12345);
  SEGGER_RTT_printf(RTTAddress, 0u, "printf Test: %%.3u,    -12345 : %.3u.\r\n", -12345);
  SEGGER_RTT_printf(RTTAddress, 0u, "printf Test: %%.6u,    -12345 : %.6u.\r\n", -12345);
  SEGGER_RTT_printf(RTTAddress, 0u, "printf Test: %%6.3u,   -12345 : %6.3u.\r\n", -12345);
  SEGGER_RTT_printf(RTTAddress, 0u, "printf Test: %%8.6u,   -12345 : %8.6u.\r\n", -12345);
  SEGGER_RTT_printf(RTTAddress, 0u, "printf Test: %%08u,    -12345 : %08u.\r\n", -12345);
  SEGGER_RTT_printf(RTTAddress, 0u, "printf Test: %%08.6u,  -12345 : %08.6u.\r\n", -12345);
  SEGGER_RTT_printf(RTTAddress, 0u, "printf Test: %%0u,     -12345 : %0u.\r\n", -12345);
  SEGGER_RTT_printf(RTTAddress, 0u, "printf Test: %%-.6u,   -12345 : %-.6u.\r\n", -12345);
  SEGGER_RTT_printf(RTTAddress, 0u, "printf Test: %%-6.3u,  -12345 : %-6.3u.\r\n", -12345);
  SEGGER_RTT_printf(RTTAddress, 0u, "printf Test: %%-8.6u,  -12345 : %-8.6u.\r\n", -12345);
  SEGGER_RTT_printf(RTTAddress, 0u, "printf Test: %%-08u,   -12345 : %-08u.\r\n", -12345);
  SEGGER_RTT_printf(RTTAddress, 0u, "printf Test: %%-08.6u, -12345 : %-08.6u.\r\n", -12345);
  SEGGER_RTT_printf(RTTAddress, 0u, "printf Test: %%-0u,    -12345 : %-0u.\r\n", -12345);

  SEGGER_RTT_printf(RTTAddress, 0u, "printf Test: %%d,      -12345 : %d.\r\n", -12345);
  SEGGER_RTT_printf(RTTAddress, 0u, "printf Test: %%+d,     -12345 : %+d.\r\n", -12345);
  SEGGER_RTT_printf(RTTAddress, 0u, "printf Test: %%.3d,    -12345 : %.3d.\r\n", -12345);
  SEGGER_RTT_printf(RTTAddress, 0u, "printf Test: %%.6d,    -12345 : %.6d.\r\n", -12345);
  SEGGER_RTT_printf(RTTAddress, 0u, "printf Test: %%6.3d,   -12345 : %6.3d.\r\n", -12345);
  SEGGER_RTT_printf(RTTAddress, 0u, "printf Test: %%8.6d,   -12345 : %8.6d.\r\n", -12345);
  SEGGER_RTT_printf(RTTAddress, 0u, "printf Test: %%08d,    -12345 : %08d.\r\n", -12345);
  SEGGER_RTT_printf(RTTAddress, 0u, "printf Test: %%08.6d,  -12345 : %08.6d.\r\n", -12345);
  SEGGER_RTT_printf(RTTAddress, 0u, "printf Test: %%0d,     -12345 : %0d.\r\n", -12345);
  SEGGER_RTT_printf(RTTAddress, 0u, "printf Test: %%-.6d,   -12345 : %-.6d.\r\n", -12345);
  SEGGER_RTT_printf(RTTAddress, 0u, "printf Test: %%-6.3d,  -12345 : %-6.3d.\r\n", -12345);
  SEGGER_RTT_printf(RTTAddress, 0u, "printf Test: %%-8.6d,  -12345 : %-8.6d.\r\n", -12345);
  SEGGER_RTT_printf(RTTAddress, 0u, "printf Test: %%-08d,   -12345 : %-08d.\r\n", -12345);
  SEGGER_RTT_printf(RTTAddress, 0u, "printf Test: %%-08.6d, -12345 : %-08.6d.\r\n", -12345);
  SEGGER_RTT_printf(RTTAddress, 0u, "printf Test: %%-0d,    -12345 : %-0d.\r\n", -12345);

  SEGGER_RTT_printf(RTTAddress, 0u, "printf Test: %%x,      0x1234ABC : %x.\r\n", 0x1234ABC);
  SEGGER_RTT_printf(RTTAddress, 0u, "printf Test: %%+x,     0x1234ABC : %+x.\r\n", 0x1234ABC);
  SEGGER_RTT_printf(RTTAddress, 0u, "printf Test: %%.3x,    0x1234ABC : %.3x.\r\n", 0x1234ABC);
  SEGGER_RTT_printf(RTTAddress, 0u, "printf Test: %%.6x,    0x1234ABC : %.6x.\r\n", 0x1234ABC);
  SEGGER_RTT_printf(RTTAddress, 0u, "printf Test: %%6.3x,   0x1234ABC : %6.3x.\r\n", 0x1234ABC);
  SEGGER_RTT_printf(RTTAddress, 0u, "printf Test: %%8.6x,   0x1234ABC : %8.6x.\r\n", 0x1234ABC);
  SEGGER_RTT_printf(RTTAddress, 0u, "printf Test: %%08x,    0x1234ABC : %08x.\r\n", 0x1234ABC);
  SEGGER_RTT_printf(RTTAddress, 0u, "printf Test: %%08.6x,  0x1234ABC : %08.6x.\r\n", 0x1234ABC);
  SEGGER_RTT_printf(RTTAddress, 0u, "printf Test: %%0x,     0x1234ABC : %0x.\r\n", 0x1234ABC);
  SEGGER_RTT_printf(RTTAddress, 0u, "printf Test: %%-.6x,   0x1234ABC : %-.6x.\r\n", 0x1234ABC);
  SEGGER_RTT_printf(RTTAddress, 0u, "printf Test: %%-6.3x,  0x1234ABC : %-6.3x.\r\n", 0x1234ABC);
  SEGGER_RTT_printf(RTTAddress, 0u, "printf Test: %%-8.6x,  0x1234ABC : %-8.6x.\r\n", 0x1234ABC);
  SEGGER_RTT_printf(RTTAddress, 0u, "printf Test: %%-08x,   0x1234ABC : %-08x.\r\n", 0x1234ABC);
  SEGGER_RTT_printf(RTTAddress, 0u, "printf Test: %%-08.6x, 0x1234ABC : %-08.6x.\r\n", 0x1234ABC);
  SEGGER_RTT_printf(RTTAddress, 0u, "printf Test: %%-0x,    0x1234ABC : %-0x.\r\n", 0x1234ABC);

  SEGGER_RTT_printf(RTTAddress, 0u, "printf Test: %%p,      &_Cnt      : %p.\r\n", &_Cnt);

  SEGGER_RTT_WriteString(RTTAddress, 0u, "###### SEGGER_printf() Tests done. ######\r\n");
  do {
    _Cnt++;
  } while (1);
}

/*************************** End of file ****************************/
