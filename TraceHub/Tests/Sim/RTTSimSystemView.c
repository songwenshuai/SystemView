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
File    : RTTSimSystemView.c
Purpose : SystemView event generation for RTT simulation programs
---------------------------END-OF-HEADER------------------------------
*/

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "SEGGER_RTT.h"
#include "SEGGER_SYSVIEW.h"
#include "SYS.h"
#include "RTTSimSystemView.h"

/*********************************************************************
*
*       Defines
*
**********************************************************************
*/

#define RTT_SIM_SYSVIEW_TASK_COUNT       3u
#define RTT_SIM_SYSVIEW_STACK_SIZE       0x1000u
#define RTT_SIM_SYSVIEW_LOCK_WAIT_MS     60000u
#define RTT_SIM_SYSVIEW_LOCK_POLL_MS     10u
#define RTT_SIM_SYSVIEW_LOCK_MAX_SPINS   100000000u

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

static RTT_SIM_SYSVIEW_CoreConfig_t _core_config;
static uintptr_t                    _rtt_address;
static uintptr_t                    _spinlock_address;
static size_t                       _spinlock_size;
static unsigned                     _spinlock_core_id;
static unsigned                     _interrupt_id;
static uint64_t                     _recorded_bytes;
static int                          _started;

/*********************************************************************
*
*       Static functions
*
**********************************************************************
*/

/*********************************************************************
*
*       _TaskId()
*
*  Function description
*    Build the deterministic SystemView task identifier for a task index.
*
*  Parameters
*    index  Task table index.
*
*  Return value
*    SystemView task identifier.
*/
static uint32_t _TaskId(unsigned index) {
    return _core_config.task_base + ((index + 1u) * 0x10u);
}

/*********************************************************************
*
*       _TaskName()
*
*  Function description
*    Get the deterministic task name for a task index.
*
*  Parameters
*    index  Task table index.
*
*  Return value
*    Task name string.
*/
static const char *_TaskName(unsigned index) {
    static const char *const names[RTT_SIM_SYSVIEW_TASK_COUNT] = {
        "worker",
        "network",
        "idle"
    };

    if (index >= RTT_SIM_SYSVIEW_TASK_COUNT) {
        return "unknown";
    }
    return names[index];
}

/*********************************************************************
*
*       _SendTaskInfo()
*
*  Function description
*    Send SystemView task metadata for a simulated task.
*
*  Parameters
*    index  Task table index.
*/
static void _SendTaskInfo(unsigned index) {
    SEGGER_SYSVIEW_TASKINFO info;
    char                    name[32];

    snprintf(name, sizeof(name), "%s.%s", _core_config.core_name, _TaskName(index));

    memset(&info, 0, sizeof(info));
    info.TaskID     = _TaskId(index);
    info.sName      = name;
    info.Prio       = (index == 2u) ? 255u : (10u + index);
    info.StackBase  = _core_config.stack_base + (index * RTT_SIM_SYSVIEW_STACK_SIZE);
    info.StackSize  = RTT_SIM_SYSVIEW_STACK_SIZE;
    info.StackUsage = 256u + (index * 96u);

    SEGGER_SYSVIEW_OnTaskCreate(info.TaskID);
    SEGGER_SYSVIEW_SendTaskInfo(&info);
    SEGGER_SYSVIEW_NameResource(info.TaskID, name);
}

/*********************************************************************
*
*       _SendCoreDescription()
*
*  Function description
*    Send SystemView system description metadata for the simulated core.
*/
static void _SendCoreDescription(void) {
    char desc[96];

    snprintf(desc,
             sizeof(desc),
             "N=%s,D=%s,O=%s",
             _core_config.application_name,
             _core_config.device_name,
             _core_config.os_name);
    SEGGER_SYSVIEW_SendSysDesc(desc);

    snprintf(desc,
             sizeof(desc),
             "I#%u=%s timer,I#%u=%s IPC",
             _core_config.interrupt_base,
             _core_config.core_name,
             _core_config.interrupt_base + 1u,
             _core_config.core_name);
    SEGGER_SYSVIEW_SendSysDesc(desc);

    SEGGER_SYSVIEW_NameMarker(_core_config.marker_base + 0u, "frame");
    SEGGER_SYSVIEW_NameMarker(_core_config.marker_base + 1u, "ipc");
    SEGGER_SYSVIEW_NameMarker(_core_config.marker_base + 2u, "io");
}

/*********************************************************************
*
*       _WaitForSpinLock()
*
*  Function description
*    Wait until the RTT software spinlock area is initialized.
*
*  Parameters
*    address  Spinlock base address.
*    size     Spinlock memory size in bytes.
*
*  Return value
*    0   Spinlock is ready.
*   -1   Spinlock did not become ready before timeout.
*/
static int _WaitForSpinLock(uintptr_t address, size_t size) {
    uint64_t start_us;
    uint64_t timeout_us;

    start_us = SYS_GetMonotonicTimeUs();
    timeout_us = (uint64_t)RTT_SIM_SYSVIEW_LOCK_WAIT_MS * 1000u;

    for (;;) {
        if (SEGGER_RTT_SPINLOCK_SW_Check(address, size) == 0) {
            return 0;
        }
        if ((SYS_GetMonotonicTimeUs() - start_us) >= timeout_us) {
            break;
        }
        SYS_Sleep(RTT_SIM_SYSVIEW_LOCK_POLL_MS);
    }

    fprintf(stderr,
            "SystemView spinlock is not ready after %u ms\n",
            RTT_SIM_SYSVIEW_LOCK_WAIT_MS);
    return -1;
}

/*********************************************************************
*
*       _ConfigureSpinLock()
*
*  Function description
*    Configure the simulated SystemView spinlock address and owner id.
*
*  Parameters
*    config  Core simulation configuration.
*
*  Return value
*    0   Spinlock configured.
*   -1   Configuration is invalid or spinlock is not ready.
*/
static int _ConfigureSpinLock(const RTT_SIM_SYSVIEW_CoreConfig_t *config) {
    size_t rtt_size;

    rtt_size = SEGGER_RTT_GetRequiredMemSize(config->num_channels);
    if ((rtt_size == 0u) || (config->rtt_region_size < rtt_size)) {
        return -1;
    }
    if (SEGGER_RTT_SPINLOCK_SW_SIZE > (config->rtt_region_size - rtt_size)) {
        return -1;
    }
    if ((config->rtt_address > (uintptr_t)(UINTPTR_MAX - rtt_size)) ||
        (config->core_id >= SEGGER_RTT_SPINLOCK_MAX_CORES)) {
        return -1;
    }

    _spinlock_address = config->rtt_address + rtt_size;
    _spinlock_size = config->rtt_region_size - rtt_size;
    _spinlock_core_id = config->core_id;

    if (config->lock_owner) {
        return SEGGER_RTT_SPINLOCK_SW_Create(_spinlock_address, _spinlock_size);
    }
    return _WaitForSpinLock(_spinlock_address, _spinlock_size);
}

/*********************************************************************
*
*       Public hooks used by SEGGER_SYSVIEW_Conf.h
*
**********************************************************************
*/

/*********************************************************************
*
*       RTT_SIM_SYSVIEW_GetRTTAddress()
*
*  Function description
*    Return the active simulated SystemView RTT control block address.
*
*  Return value
*    RTT control block address.
*/
uintptr_t RTT_SIM_SYSVIEW_GetRTTAddress(void) {
    return _rtt_address;
}

/*********************************************************************
*
*       RTT_SIM_SYSVIEW_GetTimestamp()
*
*  Function description
*    Return the current simulated SystemView timestamp.
*
*  Return value
*    Timestamp in configured SystemView ticks.
*/
unsigned RTT_SIM_SYSVIEW_GetTimestamp(void) {
    return (unsigned)(SYS_GetMonotonicTimeUs() & 0xFFFFFFFFu);
}

/*********************************************************************
*
*       RTT_SIM_SYSVIEW_GetInterruptId()
*
*  Function description
*    Return the current simulated interrupt identifier.
*
*  Return value
*    Interrupt identifier.
*/
unsigned RTT_SIM_SYSVIEW_GetInterruptId(void) {
    return _interrupt_id;
}

/*********************************************************************
*
*       RTT_SIM_SYSVIEW_OnEventRecorded()
*
*  Function description
*    Account for bytes recorded by the simulated SystemView backend.
*
*  Parameters
*    NumBytes  Number of bytes recorded.
*/
void RTT_SIM_SYSVIEW_OnEventRecorded(unsigned NumBytes) {
    _recorded_bytes += NumBytes;
}

/*********************************************************************
*
*       RTT_SIM_SYSVIEW_Lock()
*
*  Function description
*    Acquire the simulated SystemView RTT spinlock.
*/
void RTT_SIM_SYSVIEW_Lock(void) {
    if (SEGGER_RTT_SPINLOCK_SW_LockWithLimit(_spinlock_address,
                                             _spinlock_core_id,
                                             RTT_SIM_SYSVIEW_LOCK_MAX_SPINS) != 0) {
        fprintf(stderr, "SystemView spinlock lock failed\n");
        abort();
    }
}

/*********************************************************************
*
*       RTT_SIM_SYSVIEW_Unlock()
*
*  Function description
*    Release the simulated SystemView RTT spinlock.
*/
void RTT_SIM_SYSVIEW_Unlock(void) {
    if (SEGGER_RTT_SPINLOCK_SW_Unlock(_spinlock_address, _spinlock_core_id) != 0) {
        fprintf(stderr, "SystemView spinlock unlock failed\n");
        abort();
    }
}

/*********************************************************************
*
*       Public code
*
**********************************************************************
*/

/*********************************************************************
*
*       RTT_SIM_SYSVIEW_StartCore()
*
*  Function description
*    Initialize simulated SystemView state for one core and emit initial
*    target metadata.
*
*  Parameters
*    config  Core simulation configuration.
*
*  Return value
*    0   Success.
*   -1   Invalid configuration or spinlock setup failure.
*/
int RTT_SIM_SYSVIEW_StartCore(const RTT_SIM_SYSVIEW_CoreConfig_t *config) {
    SEGGER_SYSVIEW_CORE_CONTEXT *context;
    unsigned                     i;

    if ((config == NULL) ||
        (config->rtt_address == 0u) ||
        (config->rtt_region_size == 0u) ||
        (config->num_channels == 0u) ||
        (config->core_name == NULL) ||
        (config->application_name == NULL) ||
        (config->device_name == NULL) ||
        (config->os_name == NULL)) {
        return -1;
    }
    if (_ConfigureSpinLock(config) != 0) {
        return -1;
    }

    _core_config = *config;
    _rtt_address = config->rtt_address;
    _interrupt_id = config->interrupt_base;
    _recorded_bytes = 0u;

    context = SEGGER_SYSVIEW_GetMainContext();
    memset(context, 0, sizeof(*context));
    context->UpChannel        = (U8)config->channel;
    context->DownChannel      = (U8)config->channel;
    context->SysFreq          = RTT_SIM_SYSVIEW_TIMESTAMP_FREQ_HZ;
    context->CPUFreq          = config->cpu_freq_hz;
    context->RAMBaseAddress   = config->ram_base;
    context->LastTxTimeStamp  = 0u;
    context->EnableState      = 0u;

    SEGGER_SYSVIEW_Start_Ex(context, RTT_SIM_SYSVIEW_GetTimestamp());
    _SendCoreDescription();

    for (i = 0u; i < RTT_SIM_SYSVIEW_TASK_COUNT; i++) {
        _SendTaskInfo(i);
    }

    _started = 1;
    return 0;
}

/*********************************************************************
*
*       RTT_SIM_SYSVIEW_RecordCycle()
*
*  Function description
*    Record one deterministic simulated SystemView activity cycle.
*
*  Parameters
*    sequence      Simulation sequence number.
*    load_percent  Simulated load percentage.
*
*  Return value
*    Number of bytes recorded during this cycle.
*/
unsigned RTT_SIM_SYSVIEW_RecordCycle(unsigned sequence, unsigned load_percent) {
    uint64_t before_bytes;
    uint32_t task_id;
    uint32_t marker_id;
    uint32_t timer_id;
    unsigned task_index;

    if (!_started) {
        return 0u;
    }

    before_bytes = _recorded_bytes;
    task_index = sequence % RTT_SIM_SYSVIEW_TASK_COUNT;
    task_id = _TaskId(task_index);
    marker_id = _core_config.marker_base + (sequence % 3u);
    timer_id = _core_config.timer_base + (sequence % 4u);
    _interrupt_id = _core_config.interrupt_base + (sequence % 2u);

    SEGGER_SYSVIEW_RecordEnterISR();
    if ((sequence % 3u) == 0u) {
        SEGGER_SYSVIEW_RecordExitISRToScheduler();
    } else {
        SEGGER_SYSVIEW_RecordExitISR();
    }

    SEGGER_SYSVIEW_OnTaskStartReady(task_id);
    SEGGER_SYSVIEW_OnTaskStartExec(task_id);
    SEGGER_SYSVIEW_RecordEnterTimer(timer_id);
    SEGGER_SYSVIEW_MarkStart(marker_id);

    if ((sequence % 4u) == 0u) {
        SEGGER_SYSVIEW_PrintfTarget("%s event #%u load=%u%%",
                                    _core_config.core_name,
                                    sequence,
                                    load_percent);
    }

    SEGGER_SYSVIEW_RecordU32x3(32u + _core_config.core_id,
                               _core_config.core_id,
                               sequence,
                               load_percent);
    SEGGER_SYSVIEW_MarkStop(marker_id);
    SEGGER_SYSVIEW_RecordExitTimer();
    SEGGER_SYSVIEW_OnTaskStopExec();

    if ((sequence % 5u) == 0u) {
        SEGGER_SYSVIEW_OnIdle();
    }

    return (unsigned)(_recorded_bytes - before_bytes);
}

/*********************************************************************
*
*       RTT_SIM_SYSVIEW_StopCore()
*
*  Function description
*    Stop simulated SystemView recording and clear spinlock state.
*/
void RTT_SIM_SYSVIEW_StopCore(void) {
    if (_started) {
        SEGGER_SYSVIEW_Stop_Ex(SEGGER_SYSVIEW_GetMainContext());
        _started = 0;
    }
    _spinlock_address = 0u;
    _spinlock_size = 0u;
    _spinlock_core_id = 0u;
}

/*********************************************************************
*
*       RTT_SIM_SYSVIEW_GetRecordedBytes()
*
*  Function description
*    Return the total number of simulated SystemView bytes recorded.
*
*  Return value
*    Total recorded byte count.
*/
uint64_t RTT_SIM_SYSVIEW_GetRecordedBytes(void) {
    return _recorded_bytes;
}

/*************************** End of file ****************************/
