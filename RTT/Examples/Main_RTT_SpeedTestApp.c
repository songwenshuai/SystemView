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

static unsigned char _aRTTMemory[SEGGER_RTT__REQUIRED_MEM_SIZE + 4u];

/*********************************************************************
*
*       _GetRTTAddress
*/
static uintptr_t _GetRTTAddress(void) {
  return ((uintptr_t)_aRTTMemory + 3u) & ~(uintptr_t)3u;
}

/*********************************************************************
*
*       main
*
*********************************************************************/

int main(void) {
  uintptr_t RTTAddress;

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
