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
File    : LogMergerOutput.c
Purpose : Log merger detached entry delivery
---------------------------END-OF-HEADER------------------------------
*/

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include "LogMerger_internal.h"

/*********************************************************************
*
*       Public internal functions
*
**********************************************************************
*/

/*********************************************************************
*
*       _Merger_WaitForCapacityReadyEntries()
*
*  Function description
*    Wait until the configured timeout makes at least one buffered entry
*    safe to detach. The caller holds operation_mutex and mutex. The mutex
*    is released while sleeping and held again on return.
*
*  Return value
*     1  Ready entries detached
*     0  No ready entries became available
*    -1  Allocation failed
*/
int _Merger_WaitForCapacityReadyEntries(LogEntry_t ***entries, unsigned *count,
                                        const LogEntry_t *limit_entry) {
    unsigned wait_start_tick;
    unsigned timeout_ms;

    *entries = NULL;
    *count   = 0u;

    timeout_ms = _merger_state.flush_timeout_ms;
    if (timeout_ms == 0u) {
        return 0;
    }

    wait_start_tick = SYS_GetTickCount();
    for (;;) {
        unsigned now;
        unsigned elapsed;
        unsigned wait_ms;

        if (_Merger_DetachReadyEntriesLimited(entries, count, limit_entry) != 0) {
            return -1;
        }
        if (*count > 0u) {
            return 1;
        }

        now = SYS_GetTickCount();
        elapsed = (unsigned)(now - wait_start_tick);
        if (elapsed >= timeout_ms) {
            return 0;
        }

        wait_ms = timeout_ms - elapsed;
        if (wait_ms > LOG_MERGER_CAPACITY_WAIT_SLICE_MS) {
            wait_ms = LOG_MERGER_CAPACITY_WAIT_SLICE_MS;
        }
        if (wait_ms == 0u) {
            wait_ms = 1u;
        }

        SYS_MutexUnlock(&_merger_state.mutex);
        SYS_Sleep(wait_ms);
        SYS_MutexLock(&_merger_state.mutex);
    }
}

/*********************************************************************
*
*       _Merger_DeliverDetachedEntries()
*
*  Function description
*    Deliver detached entries in order.
*
*  Parameters
*    entries    Detached entry pointer array
*    count      Number of entries in array
*    output     Output callback
*    user_data  User context for callback
*    stopped    Receives callback stop request state
*
*  Return value
*    Number of entries successfully delivered.
*/
unsigned _Merger_DeliverDetachedEntries(LogEntry_t **entries, unsigned count,
                                        LogMerger_Output_t output, void *user_data,
                                        bool *stopped) {
    unsigned i;
    unsigned flushed;

    *stopped = false;
    flushed  = 0;

    for (i = 0; i < count; i++) {
        LogEntry_t *entry;
        int         result;
        uint64_t    timestamp;
        uint64_t    sequence;
        LogSource_t source;

        if (entries[i] == NULL) {
            continue;
        }

        entry      = entries[i];
        entries[i] = NULL;
        timestamp  = LogEntry_GetTimestamp(entry);
        source     = LogEntry_GetSource(entry);
        sequence   = LogEntry_GetSequence(entry);
        result     = output(entry, user_data);
        if (result != 0) {
            unsigned j;

            *stopped = true;
            for (j = i + 1; j < count; j++) {
                if (entries[j] != NULL) {
                    LogEntry_Destroy(entries[j]);
                    entries[j] = NULL;
                }
            }
            break;
        }

        flushed++;
        _Merger_RecordDeliveredKey(timestamp, source, sequence);
    }

    return flushed;
}

/*************************** End of file ****************************/
