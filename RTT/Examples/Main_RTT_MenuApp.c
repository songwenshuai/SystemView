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
*/
void main(void) {
  uintptr_t RTTAddress;
  int r;
  int CancelOp;

  RTTAddress = _GetRTTAddress();
  SEGGER_RTT_Init(RTTAddress);
  do {
    _Cnt = 0;

    SEGGER_RTT_WriteString(RTTAddress, 0u, "SEGGER Real-Time-Terminal Sample\r\n");
    SEGGER_RTT_WriteString(RTTAddress, 0u, "Press <1> to continue in blocking mode (Application waits if necessary, no data lost)\r\n");
    SEGGER_RTT_WriteString(RTTAddress, 0u, "Press <2> to continue in non-blocking mode (Application does not wait, data lost if fifo full)\r\n");
    do {
      r = SEGGER_RTT_WaitKey(RTTAddress);
    } while ((r != '1') && (r != '2'));
    if (r == '1') {
      SEGGER_RTT_WriteString(RTTAddress, 0u, "\r\nSelected <1>. Configuring RTT and starting...\r\n");
      SEGGER_RTT_ConfigUpBuffer(RTTAddress, 0u, NULL, NULL, 0, SEGGER_RTT_MODE_BLOCK_IF_FIFO_FULL);
    } else {
      SEGGER_RTT_WriteString(RTTAddress, 0u, "\r\nSelected <2>. Configuring RTT and starting...\r\n");
      SEGGER_RTT_ConfigUpBuffer(RTTAddress, 0u, NULL, NULL, 0, SEGGER_RTT_MODE_NO_BLOCK_SKIP);
    }
    CancelOp = 0;
    do {
      //for (_Delay = 0; _Delay < 10000; _Delay++);
      SEGGER_RTT_printf(RTTAddress, 0u, "Count: %d. Press <Space> to get back to menu.\r\n", _Cnt++);
      r = SEGGER_RTT_HasKey(RTTAddress);
      if (r) {
        CancelOp = (SEGGER_RTT_GetKey(RTTAddress) == ' ') ? 1 : 0;
      }
      //
      // Check if user selected to cancel the current operation
      //
      if (CancelOp) {
        SEGGER_RTT_WriteString(RTTAddress, 0u, "Operation cancelled, going back to menu...\r\n");
        break;
      }
    } while (1);
    SEGGER_RTT_GetKey(RTTAddress);
    SEGGER_RTT_WriteString(RTTAddress, 0u, "\r\n");
  } while (1);
}

/*************************** End of file ****************************/
