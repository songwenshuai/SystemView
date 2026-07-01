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
File    : LogMergerWatermark.c
Purpose : Log merger source watermark and delivery ordering rules
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
*       Static functions
*
**********************************************************************
*/

/*********************************************************************
*
*       _SourceIsRequired()
*
*  Function description
*    Check whether a source participates in merge ordering.
*/
static bool _SourceIsRequired(LogSource_t source) {
    if (source >= LOG_SOURCE_MAX) {
        return false;
    }

    return _merger_state.config.required_source[source];
}

/*********************************************************************
*
*       Public internal functions
*
**********************************************************************
*/

/*********************************************************************
*
*       _Merger_EntrySourceIsRequired()
*
*  Function description
*    Check whether an entry source participates in merge ordering.
*/
bool _Merger_EntrySourceIsRequired(const LogEntry_t *entry) {
    if (!LogEntry_IsValid(entry)) {
        return false;
    }

    return _SourceIsRequired(LogEntry_GetSource(entry));
}

/*********************************************************************
*
*       _Merger_GetReadyWatermark()
*
*  Function description
*    Get the timestamp watermark for entries that can be delivered.
*    Required sources that have not appeared yet block delivery until the
*    oldest buffered entry has waited past flush_timeout_ms. Required sources
*    that have appeared but then go idle are ignored after the same timeout.
*
*  Return value
*    true   Watermark available
*    false  No buffered entry is available
*/
bool _Merger_GetReadyWatermark(uint64_t *watermark) {
    uint64_t result;
    unsigned now;
    unsigned timeout_ms;
    unsigned i;
    bool     have_active_source;
    bool     buffer_wait_expired;

    if (watermark == NULL) {
        return false;
    }
    if (_merger_state.entry_count == 0) {
        return false;
    }

    result = UINT64_MAX;
    now = SYS_GetTickCount();
    timeout_ms = _merger_state.flush_timeout_ms;
    have_active_source = false;
    buffer_wait_expired = timeout_ms > 0u &&
                          _merger_state.first_buffered_tick != 0u &&
                          (unsigned)(now - _merger_state.first_buffered_tick) >= timeout_ms;

    for (i = 0; i < LOG_SOURCE_MAX; i++) {
        if (!_merger_state.config.required_source[i]) {
            continue;
        }
        if (!_merger_state.source_seen[i]) {
            if (!buffer_wait_expired) {
                return false;
            }
            continue;
        }
        if (timeout_ms > 0 &&
            (unsigned)(now - _merger_state.source_last_tick[i]) >= timeout_ms) {
            continue;
        }
        have_active_source = true;
        if (_merger_state.source_watermark[i] < result) {
            result = _merger_state.source_watermark[i];
        }
    }

    if (!have_active_source) {
        result = LogEntry_GetTimestamp(_merger_state.buffer[_merger_state.entry_count - 1]);
    }

    *watermark = result;
    return true;
}

/*********************************************************************
*
*       _Merger_EntryIsBeforeLastDelivered()
*
*  Function description
*    Check whether an entry would violate delivered output ordering.
*/
bool _Merger_EntryIsBeforeLastDelivered(const LogEntry_t *entry) {
    uint64_t    timestamp;
    uint64_t    sequence;
    LogSource_t source;

    if (!_merger_state.last_delivered_valid) {
        return false;
    }

    timestamp = LogEntry_GetTimestamp(entry);
    source    = LogEntry_GetSource(entry);
    sequence  = LogEntry_GetSequence(entry);

    if (timestamp < _merger_state.last_delivered_timestamp) {
        return true;
    }
    if (timestamp > _merger_state.last_delivered_timestamp) {
        return false;
    }

    if (source < _merger_state.last_delivered_source) {
        return true;
    }
    if (source > _merger_state.last_delivered_source) {
        return false;
    }

    return sequence <= _merger_state.last_delivered_sequence;
}

/*********************************************************************
*
*       _Merger_UpdateSourceWatermark()
*
*  Function description
*    Record the newest timestamp seen for an entry source.
*
*  Return value
*    0   Success
*   -1   Entry source is disabled or would violate already delivered ordering
*/
int _Merger_UpdateSourceWatermark(const LogEntry_t *entry) {
    uint64_t    timestamp;
    LogSource_t source;
    unsigned    now;

    if (!_Merger_EntrySourceIsRequired(entry) ||
        _Merger_EntryIsBeforeLastDelivered(entry)) {
        return -1;
    }

    timestamp = LogEntry_GetTimestamp(entry);
    source    = LogEntry_GetSource(entry);
    now       = SYS_GetTickCount();

    if (!_merger_state.source_seen[source] ||
        timestamp > _merger_state.source_watermark[source]) {
        _merger_state.source_watermark[source] = timestamp;
    }
    _merger_state.source_last_tick[source] = now;
    _merger_state.source_seen[source] = true;

    return 0;
}

/*********************************************************************
*
*       _Merger_RecordDeliveredKey()
*
*  Function description
*    Record the ordering key of an entry delivered successfully.
*/
void _Merger_RecordDeliveredKey(uint64_t timestamp, LogSource_t source, uint64_t sequence) {
    _merger_state.last_delivered_timestamp = timestamp;
    _merger_state.last_delivered_source    = source;
    _merger_state.last_delivered_sequence  = sequence;
    _merger_state.last_delivered_valid     = true;
}

/*************************** End of file ****************************/
