/*********************************************************************
*                             CineLogic                              *
*                      RTT Trace and Debug Bridge                    *
**********************************************************************
*                                                                    *
*                    (c) 2023 - 2026 CineLogic                       *
*                                                                    *
*                  Support: wenshuaisong@gmail.com                   *
*                                                                    *
**********************************************************************
*                                                                    *
*       CineLogic TraceHub * RTT trace and debug bridge              *
*                                                                    *
**********************************************************************
*                                                                    *
* All rights reserved.                                               *
*                                                                    *
* CineLogic strongly recommends to not make any changes              *
* to or modify the source code of this software in order to stay     *
* compatible with the SharedMem and RTT data path.                   *
*                                                                    *
* Redistribution and use in source and binary forms, with or         *
* without modification, are permitted provided that the following    *
* condition is met:                                                  *
*                                                                    *
* o Redistributions of source code must retain the above copyright   *
*   notice, this condition and the following disclaimer.             *
*                                                                    *
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND             *
* CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,        *
* INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF           *
* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE           *
* DISCLAIMED. IN NO EVENT SHALL CINELOGIC BE LIABLE FOR              *
* ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR           *
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT  *
* OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;    *
* OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF      *
* LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT          *
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE  *
* USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH   *
* DAMAGE.                                                            *
*                                                                    *
**********************************************************************
----------------------------------------------------------------------
File    : SEGGER_SYSVIEW_Conf.h
Purpose : SystemView configuration for RTT simulation programs
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
