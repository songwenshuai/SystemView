/*********************************************************************
*                   (c) SEGGER Microcontroller GmbH                  *
*                        The Embedded Experts                        *
*                           www.segger.com                           *
**********************************************************************
*                                                                    *
*         SEGGER SystemView  * Real-time application analysis        *
*              https://github.com/SEGGERMicro/SystemView             *
*                                                                    *
**********************************************************************

---------------------------END-OF-HEADER------------------------------

Purpose : SEGGER SystemView configuration file.
          Set defines which deviate from the defaults (see SEGGER_SYSVIEW_ConfDefaults.h) here.

Additional information:
  Required defines which must be set are:
    SEGGER_SYSVIEW_GET_TIMESTAMP
    SEGGER_SYSVIEW_GET_INTERRUPT_ID
  For known compilers and cores, these might be set to good defaults
  in SEGGER_SYSVIEW_ConfDefaults.h.

  SystemView needs a (nestable) locking mechanism.
  If not defined, the RTT locking mechanism is used,
  which then needs to be properly configured.
*/

#ifndef SEGGER_SYSVIEW_CONF_H
#define SEGGER_SYSVIEW_CONF_H

#include "SEGGER_RTT_Memory.h"

/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/

/*********************************************************************
*
*       Define: SEGGER_SYSVIEW_RTT_*_ADDRESS
*
*  Description
*    Shared-memory addresses used by the SystemView RTT transport.
*  Notes
*    All addresses point into the local MEMSHM mapping of /rtt_sim.
*/

#define SEGGER_SYSVIEW_RTT_CB_ADDRESS           SEGGER_SIM_RTT_GetMemoryAddress()
#define SEGGER_SYSVIEW_RTT_NAME_ADDRESS         (0u)
#define SEGGER_SYSVIEW_RTT_CHANNEL              SEGGER_SIM_SYSVIEW_CHANNEL
#define SEGGER_SYSVIEW_RTT_UP_BUFFER_ADDRESS    SEGGER_SIM_RTT_GetSystemViewUpBufferAddress()
#define SEGGER_SYSVIEW_RTT_DOWN_BUFFER_ADDRESS  SEGGER_SIM_RTT_GetSystemViewDownBufferAddress()
#define SEGGER_SYSVIEW_RTT_BUFFER_SIZE          SEGGER_SIM_SYSVIEW_RTT_BUFFER_SIZE
#define SEGGER_SYSVIEW_RTT_DOWN_BUFFER_SIZE     SEGGER_SIM_SYSVIEW_RTT_DOWN_BUFFER_SIZE
#define SEGGER_SYSVIEW_CPU_CACHE_LINE_SIZE      0
#define SEGGER_SYSVIEW_APP_NAME                 "embOS start project"
#define SEGGER_SYSVIEW_DEVICE_NAME              "Simulation"

#endif  // SEGGER_SYSVIEW_CONF_H

/*************************** End of file ****************************/
