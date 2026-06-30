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
----------------------------------------------------------------------
File    : SEGGER_RTT_Memory_Int.h
Purpose : Internal MEMSHM RTT memory layout definitions for embOS
          simulation.
---------------------------END-OF-HEADER------------------------------
*/

#ifndef SEGGER_RTT_MEMORY_INT_H
#define SEGGER_RTT_MEMORY_INT_H

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include "SEGGER_RTT_Memory.h"
#include "SEGGER_RTT.h"

/*********************************************************************
*
*       Defines, fixed
*
**********************************************************************
*/

#if (SEGGER_SIM_RTT_NUM_CHANNELS == 0u)
  #error "SEGGER_SIM_RTT_NUM_CHANNELS must not be 0"
#endif

#if (SEGGER_SIM_RTT_NUM_CHANNELS > SEGGER_RTT_MAX_NUM_UP_BUFFERS) || (SEGGER_SIM_RTT_NUM_CHANNELS > SEGGER_RTT_MAX_NUM_DOWN_BUFFERS)
  #error "SEGGER_SIM_RTT_NUM_CHANNELS exceeds SEGGER RTT buffer limits"
#endif

#if (SEGGER_SIM_SYSVIEW_CHANNEL == 0u)
  #error "SEGGER_SIM_SYSVIEW_CHANNEL must not use the terminal RTT channel"
#endif

#if (SEGGER_SIM_SYSVIEW_CHANNEL >= SEGGER_SIM_RTT_NUM_CHANNELS)
  #error "SEGGER_SIM_SYSVIEW_CHANNEL must select a configured RTT buffer pair"
#endif

#if (SEGGER_SIM_SYSVIEW_RTT_BUFFER_SIZE > BUFFER_SIZE_UP)
  #error "SEGGER_SIM_SYSVIEW_RTT_BUFFER_SIZE exceeds the RTT up-buffer size"
#endif

#if (SEGGER_SIM_SYSVIEW_RTT_DOWN_BUFFER_SIZE > BUFFER_SIZE_DOWN)
  #error "SEGGER_SIM_SYSVIEW_RTT_DOWN_BUFFER_SIZE exceeds the RTT down-buffer size"
#endif

#define SEGGER_SIM_RTT_BUFFER_PAIRS          SEGGER_SIM_RTT_NUM_CHANNELS
#define SEGGER_SIM_RTT_REQUIRED_MEMORY_SIZE  (SEGGER_RTT_REQUIRED_MEM_SIZE_FOR_BUFFER_PAIRS(SEGGER_SIM_RTT_BUFFER_PAIRS) + SEGGER_RTT_SPINLOCK_SW_SIZE)
#define SEGGER_SIM_SYSVIEW_PAIR_BASE_OFF     SEGGER_RTT_REQUIRED_MEM_SIZE_FOR_BUFFER_PAIRS(SEGGER_SIM_SYSVIEW_CHANNEL)
#define SEGGER_SIM_SYSVIEW_UP_BUFFER_OFF     (SEGGER_SIM_SYSVIEW_PAIR_BASE_OFF + SEGGER_RTT__TERMINAL_NAME_SIZE_ALIGNED)
#define SEGGER_SIM_SYSVIEW_DOWN_BUFFER_OFF   (SEGGER_SIM_SYSVIEW_UP_BUFFER_OFF + BUFFER_SIZE_UP + SEGGER_RTT__TERMINAL_NAME_SIZE_ALIGNED)

#endif

/*************************** End of file ****************************/
