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
Purpose : Sample program for measuring RTT performance.

----------------------------------------------------------------------
*/

#include "SEGGER_RTT.h"
#include <stdio.h>

volatile int _Cnt;
volatile int _Delay;
volatile int _Marker;

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
*
*********************************************************************/

int main(void) {
  PTR_ADDR RTTAddress;

  RTTAddress = _GetRTTAddress();
  SEGGER_RTT_Init(RTTAddress);
  do {
    //
    // Measure time needed for RTT output.
    // The marker variable replaces board LED toggling so the example can
    // be built without RTOS/BSP dependencies.
    //
    _Marker = 0;
    SEGGER_RTT_Write(RTTAddress, 0u, NULL, 0u);
    _Marker = 1;
    _Marker = 0;
    SEGGER_RTT_Write(RTTAddress, 0u, "01234567890123456789012345678901234567890123456789012345678901234567890123456789\r\n", 82u);
    _Marker = 1;
    _Cnt++;
    for (_Delay = 0; _Delay < 100000; _Delay++) {
      ;
    }
  } while (1);
}
