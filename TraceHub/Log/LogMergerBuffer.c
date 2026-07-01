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
File    : LogMergerBuffer.c
Purpose : Log merger ordered buffer management
---------------------------END-OF-HEADER------------------------------
*/

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include <stdlib.h>
#include <string.h>

#include "LogMerger_internal.h"

/*********************************************************************
*
*       Static functions
*
**********************************************************************
*/

/*********************************************************************
*
*       _InsertSorted()
*
*  Function description
*    Insert entry into sorted position in buffer.
*    Uses insertion sort for small buffers.
*
*  Parameters
*    entry  Entry to insert
*
*  Return value
*    0   Success
*   -1   Buffer full
*/
static int _InsertSorted(LogEntry_t *entry) {
    unsigned i;
    unsigned insert_pos;
    //
    // Check if buffer is full
    //
    if (_merger_state.entry_count >= _merger_state.buffer_size) {
        return -1;
    }
    //
    // Find insertion position.
    //
    insert_pos = _merger_state.entry_count;

    for (i = 0; i < _merger_state.entry_count; i++) {
        if (LogEntry_Compare(entry, _merger_state.buffer[i]) < 0) {
            insert_pos = i;
            break;
        }
    }
    //
    // Shift entries to make room
    //
    for (i = _merger_state.entry_count; i > insert_pos; i--) {
        _merger_state.buffer[i] = _merger_state.buffer[i - 1];
    }
    //
    // Insert entry
    //
    _merger_state.buffer[insert_pos] = entry;
    _merger_state.entry_count++;

    return 0;
}

/*********************************************************************
*
*       _RemoveBufferedEntry()
*
*  Function description
*    Remove one buffered entry pointer without destroying it.
*    The caller must hold the merger mutex.
*/
static void _RemoveBufferedEntry(LogEntry_t *entry) {
    unsigned i;
    unsigned remaining_count;

    if (entry == NULL) {
        return;
    }

    for (i = 0; i < _merger_state.entry_count; i++) {
        if (_merger_state.buffer[i] != entry) {
            continue;
        }

        remaining_count = _merger_state.entry_count - i - 1u;
        if (remaining_count > 0u) {
            memmove(&_merger_state.buffer[i],
                    &_merger_state.buffer[i + 1u],
                    sizeof(LogEntry_t *) * remaining_count);
        }
        _merger_state.entry_count--;
        _merger_state.buffer[_merger_state.entry_count] = NULL;
        if (_merger_state.entry_count == 0u) {
            _merger_state.first_buffered_tick = 0u;
        }
        return;
    }
}

/*********************************************************************
*
*       Public internal functions
*
**********************************************************************
*/

/*********************************************************************
*
*       _Merger_InsertEntryAndUpdateWatermark()
*
*  Function description
*    Insert an entry and update the source watermark as one ordered state
*    transition. The caller must hold the merger mutex.
*
*  Return value
*     0  Success
*    -1  Buffer full
*    -2  Entry source is disabled or would violate already delivered ordering
*/
int _Merger_InsertEntryAndUpdateWatermark(LogEntry_t *entry) {
    unsigned now;
    bool     was_empty;

    if (_Merger_EntryIsBeforeLastDelivered(entry)) {
        return -2;
    }

    now       = SYS_GetTickCount();
    was_empty = (_merger_state.entry_count == 0u);
    if (_InsertSorted(entry) != 0) {
        return -1;
    }
    if (_Merger_UpdateSourceWatermark(entry) != 0) {
        _RemoveBufferedEntry(entry);
        return -2;
    }

    if (was_empty || _merger_state.first_buffered_tick == 0u) {
        _merger_state.first_buffered_tick = now;
    }
    _merger_state.last_insert_tick = now;
    return 0;
}

/*********************************************************************
*
*       _Merger_DetachBufferedEntries()
*
*  Function description
*    Detach buffered entries into a temporary array.
*    The caller must hold the merger mutex.
*
*  Parameters
*    entries  Receives detached entry pointer array
*    count    Receives detached entry count
*
*  Return value
*    0   Success
*   -1   Allocation failed
*/
int _Merger_DetachBufferedEntries(LogEntry_t ***entries, unsigned *count) {
    LogEntry_t **temp_buffer;

    *entries = NULL;
    *count   = _merger_state.entry_count;

    if (*count == 0) {
        return 0;
    }

    temp_buffer = (LogEntry_t **)malloc(sizeof(LogEntry_t *) * (*count));
    if (temp_buffer == NULL) {
        return -1;
    }

    memcpy(temp_buffer, _merger_state.buffer, sizeof(LogEntry_t *) * (*count));
    memset(_merger_state.buffer, 0, sizeof(LogEntry_t *) * (*count));
    _merger_state.entry_count = 0;
    _merger_state.first_buffered_tick = 0u;

    *entries = temp_buffer;
    return 0;
}

/*********************************************************************
*
*       _Merger_DetachReadyEntriesLimited()
*
*  Function description
*    Detach buffered entries whose timestamps are not newer than the
*    current source watermark. The caller must hold the merger mutex.
*
*  Parameters
*    entries      Receives detached entry pointer array
*    count        Receives detached entry count
*    limit_entry  Pending entry that bounds delivered ordering
*
*  Return value
*    0   Success
*   -1   Allocation failed
*/
int _Merger_DetachReadyEntriesLimited(LogEntry_t ***entries, unsigned *count,
                                      const LogEntry_t *limit_entry) {
    LogEntry_t **temp_buffer;
    uint64_t     watermark;
    unsigned     ready_count;
    unsigned     remaining_count;

    *entries = NULL;
    *count   = 0;

    if (_merger_state.entry_count == 0) {
        return 0;
    }
    if (!_Merger_GetReadyWatermark(&watermark)) {
        return 0;
    }

    ready_count = 0;
    while (ready_count < _merger_state.entry_count &&
           LogEntry_GetTimestamp(_merger_state.buffer[ready_count]) <= watermark) {
        if (limit_entry != NULL &&
            LogEntry_Compare(_merger_state.buffer[ready_count], limit_entry) > 0) {
            break;
        }
        ready_count++;
    }
    if (ready_count == 0) {
        return 0;
    }

    temp_buffer = (LogEntry_t **)malloc(sizeof(LogEntry_t *) * ready_count);
    if (temp_buffer == NULL) {
        return -1;
    }

    memcpy(temp_buffer, _merger_state.buffer, sizeof(LogEntry_t *) * ready_count);

    remaining_count = _merger_state.entry_count - ready_count;
    if (remaining_count > 0) {
        memmove(_merger_state.buffer,
                &_merger_state.buffer[ready_count],
                sizeof(LogEntry_t *) * remaining_count);
    }
    memset(&_merger_state.buffer[remaining_count],
           0,
           sizeof(LogEntry_t *) * ready_count);
    _merger_state.entry_count = remaining_count;
    if (_merger_state.entry_count == 0u) {
        _merger_state.first_buffered_tick = 0u;
    } else {
        _merger_state.first_buffered_tick = SYS_GetTickCount();
    }

    *entries = temp_buffer;
    *count   = ready_count;
    return 0;
}

/*********************************************************************
*
*       _Merger_DetachReadyEntries()
*
*  Function description
*    Detach watermark-ready buffered entries.
*    The caller must hold the merger mutex.
*/
int _Merger_DetachReadyEntries(LogEntry_t ***entries, unsigned *count) {
    return _Merger_DetachReadyEntriesLimited(entries, count, NULL);
}

/*************************** End of file ****************************/
