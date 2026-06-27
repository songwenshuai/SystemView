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
File    : RTTSimSystemView.h
Purpose : SystemView event generation for RTT simulation programs
---------------------------END-OF-HEADER------------------------------
*/

#ifndef RTT_SIM_SYSVIEW_H
#define RTT_SIM_SYSVIEW_H

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/*********************************************************************
*
*       Defines
*
**********************************************************************
*/

#define RTT_SIM_SYSVIEW_TIMESTAMP_FREQ_HZ     1000000u

/*********************************************************************
*
*       Types
*
**********************************************************************
*/

typedef struct {
    uintptr_t    rtt_address;
    size_t       rtt_region_size;
    unsigned     channel;
    unsigned     num_channels;
    unsigned     core_id;
    bool         lock_owner;
    const char  *core_name;
    const char  *application_name;
    const char  *device_name;
    const char  *os_name;
    uint32_t     cpu_freq_hz;
    uint32_t     ram_base;
    uint32_t     task_base;
    uint32_t     stack_base;
    uint32_t     marker_base;
    uint32_t     timer_base;
    unsigned     interrupt_base;
} RTT_SIM_SYSVIEW_CoreConfig_t;

/*********************************************************************
*
*       API functions
*
**********************************************************************
*/

int      RTT_SIM_SYSVIEW_StartCore(const RTT_SIM_SYSVIEW_CoreConfig_t *config);
unsigned RTT_SIM_SYSVIEW_RecordCycle(unsigned sequence, unsigned load_percent);
void     RTT_SIM_SYSVIEW_StopCore(void);
uint64_t RTT_SIM_SYSVIEW_GetRecordedBytes(void);
void     RTT_SIM_SYSVIEW_Lock(void);
void     RTT_SIM_SYSVIEW_Unlock(void);

#endif

/*************************** End of file ****************************/
