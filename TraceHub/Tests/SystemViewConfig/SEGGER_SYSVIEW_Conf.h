/*********************************************************************
*                                                                    *
*                Copyright (C) 2023 xrTest Inc.                      *
*                      All rights reserved                           *
*                                                                    *
**********************************************************************
----------------------------------------------------------------------
File    : SEGGER_SYSVIEW_Conf.h
Purpose : SystemView configuration for RTT simulation programs
Author  : songwenshuai <songwenshuai@gmail.com>
---------------------------END-OF-HEADER------------------------------
*/

#ifndef SEGGER_SYSVIEW_CONF_H
#define SEGGER_SYSVIEW_CONF_H

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include <stdint.h>

/*********************************************************************
*
*       External simulation hooks
*
**********************************************************************
*/

uintptr_t RTT_SIM_SYSVIEW_GetRTTAddress(void);
unsigned  RTT_SIM_SYSVIEW_GetTimestamp(void);
unsigned  RTT_SIM_SYSVIEW_GetInterruptId(void);
void      RTT_SIM_SYSVIEW_OnEventRecorded(unsigned NumBytes);
void      RTT_SIM_SYSVIEW_Lock(void);
void      RTT_SIM_SYSVIEW_Unlock(void);

/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/

#define SEGGER_SYSVIEW_CORE                    0
#define SEGGER_SYSVIEW_RTT_CB_ADDRESS          RTT_SIM_SYSVIEW_GetRTTAddress()
#define SEGGER_SYSVIEW_RTT_NAME_ADDRESS        0u
#define SEGGER_SYSVIEW_RTT_CHANNEL             2
#define SEGGER_SYSVIEW_RTT_BUFFER_SIZE         4096
#define SEGGER_SYSVIEW_MAX_STRING_LEN          96
#define SEGGER_SYSVIEW_MAX_ARGUMENTS           16
#define SEGGER_SYSVIEW_CPU_CACHE_LINE_SIZE     0
#define SEGGER_SYSVIEW_PRINTF_IMPLICIT_FORMAT  0

#define SEGGER_SYSVIEW_LOCK()                  RTT_SIM_SYSVIEW_Lock()
#define SEGGER_SYSVIEW_UNLOCK()                RTT_SIM_SYSVIEW_Unlock()
#define SEGGER_SYSVIEW_GET_TIMESTAMP()         RTT_SIM_SYSVIEW_GetTimestamp()
#define SEGGER_SYSVIEW_GET_INTERRUPT_ID()      RTT_SIM_SYSVIEW_GetInterruptId()
#define SEGGER_SYSVIEW_ON_EVENT_RECORDED(NumBytes) RTT_SIM_SYSVIEW_OnEventRecorded((unsigned)(NumBytes))

#endif

/*************************** End of file ****************************/
