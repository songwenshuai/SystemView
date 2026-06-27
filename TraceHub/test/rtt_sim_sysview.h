/*********************************************************************
*                                                                    *
*                Copyright (C) 2023 xrTest Inc.                      *
*                      All rights reserved                           *
*                                                                    *
**********************************************************************
----------------------------------------------------------------------
File    : rtt_sim_sysview.h
Purpose : SystemView event generation for RTT simulation programs
Author  : songwenshuai <songwenshuai@gmail.com>
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
