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
File    : SEGGER_RTT_Memory.h
Purpose : MEMSHM RTT memory ownership for embOS simulation.
---------------------------END-OF-HEADER------------------------------
*/

#ifndef SEGGER_RTT_MEMORY_H
#define SEGGER_RTT_MEMORY_H

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
  extern "C" {
#endif

/*********************************************************************
*
*       Defines, fixed
*
**********************************************************************
*/

#define SEGGER_SIM_RTT_SHM_NAME                  "/rtt_sim"
#define SEGGER_SIM_RTT_BACKEND_BASE             UINT64_C(0x10000000)
#define SEGGER_SIM_RTT_NUM_CHANNELS             (3u)
#define SEGGER_SIM_SYSVIEW_CHANNEL              (2u)
#define SEGGER_SIM_SYSVIEW_RTT_BUFFER_SIZE      (64u * 1024u)
#define SEGGER_SIM_SYSVIEW_RTT_DOWN_BUFFER_SIZE (1024u)

/*********************************************************************
*
*       API functions
*
**********************************************************************
*/

int       SEGGER_SIM_RTT_EnsureMemory                   (void);
void      SEGGER_SIM_RTT_CleanupMemory                  (void);
void      SEGGER_SIM_RTT_Lock                           (void);
void      SEGGER_SIM_RTT_Unlock                         (void);
uintptr_t SEGGER_SIM_RTT_GetMemoryAddress               (void);
size_t    SEGGER_SIM_RTT_GetMemorySize                  (void);
uintptr_t SEGGER_SIM_RTT_GetSystemViewUpBufferAddress   (void);
uintptr_t SEGGER_SIM_RTT_GetSystemViewDownBufferAddress (void);

#ifdef __cplusplus
  }
#endif

#endif

/*************************** End of file ****************************/
