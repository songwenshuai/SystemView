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
Purpose : User configuration file for RTT.
          For available configuration,
          refer to SEGGER_RTT_ConfDefaults.h.

----------------------------------------------------------------------
*/

#ifndef SEGGER_RTT_CONF_H
#define SEGGER_RTT_CONF_H

#include "SEGGER_RTT_Memory.h"

/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/

#define SEGGER_RTT_CB_ADDRESS                   SEGGER_SIM_RTT_GetMemoryAddress()
#define SEGGER_RTT_MAX_NUM_UP_BUFFERS           (3)
#define SEGGER_RTT_MAX_NUM_DOWN_BUFFERS         (3)
#define BUFFER_SIZE_UP                          (1048576)
#define BUFFER_SIZE_DOWN                        (1048576)
#define SEGGER_RTT_PRINTF_BUFFER_SIZE           (128u)
#define SEGGER_RTT_MODE_DEFAULT                 SEGGER_RTT_MODE_BLOCK_IF_FIFO_FULL
#define SEGGER_RTT_LOCK()                       SEGGER_SIM_RTT_Lock()
#define SEGGER_RTT_UNLOCK()                     SEGGER_SIM_RTT_Unlock()

#endif
/*************************** End of file ****************************/
