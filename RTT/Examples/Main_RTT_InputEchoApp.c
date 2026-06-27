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
volatile int _Delay;

static char r;
static unsigned char _aRTTMemory[SEGGER_RTT__REQUIRED_MEM_SIZE + SEGGER_RTT__CB_ALIGNMENT];

/*********************************************************************
*
*       _GetRTTAddress
*/
static uintptr_t _GetRTTAddress(void) {
  return ((uintptr_t)_aRTTMemory + SEGGER_RTT__CB_ALIGNMENT_MASK) & ~(uintptr_t)SEGGER_RTT__CB_ALIGNMENT_MASK;
}

/*********************************************************************
*
*       main
*/
void main(void) {
  uintptr_t RTTAddress;

  RTTAddress = _GetRTTAddress();
  SEGGER_RTT_Init(RTTAddress);
  SEGGER_RTT_WriteString(RTTAddress, 0u, "SEGGER Real-Time-Terminal Sample\r\n");
  SEGGER_RTT_ConfigUpBuffer(RTTAddress, 0u, NULL, NULL, 0, SEGGER_RTT_MODE_NO_BLOCK_SKIP);
  do {
    r = SEGGER_RTT_WaitKey(RTTAddress);
    SEGGER_RTT_Write(RTTAddress, 0u, &r, 1);
    r++;
  } while (1);
}

/*************************** End of file ****************************/
