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
----------------------------------------------------------------------
File    : CoreLogRecorderStats.c
Purpose : Core log recorder statistics reporting
---------------------------END-OF-HEADER------------------------------
*/

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "CoreLogRecorder_internal.h"
#include "RTTBridge.h"

/*********************************************************************
*
*       _Recorder_ReportStats()
*
*  Function description
*    Print per-core consumer queue delivery failure statistics.
*/
void _Recorder_ReportStats(void) {
    unsigned i;

    for (i = 0u; i < CORE_LOG_RECORDER_NUM_SOURCES; i++) {
        CoreLogRecorder_Source_t *source = &_recorder_state.sources[i];
        uint64_t                  undelivered_bytes;
        unsigned                  failure_count;

        if (!source->enabled || !source->lock_initialized) {
            continue;
        }

        SYS_MutexLock(&source->lock);
        undelivered_bytes = source->consumer_undelivered_bytes;
        failure_count = source->consumer_capacity_failure_count;
        SYS_MutexUnlock(&source->lock);

        if (undelivered_bytes > 0u) {
            fprintf(stderr,
                    "[CoreLogRecorder] %s consumer could not deliver %llu bytes in %u queue capacity failures\n",
                    source->name,
                    (unsigned long long)undelivered_bytes,
                    failure_count);
        }
    }
}


/*********************************************************************
*
*       CoreLogRecorder_GetStats()
*
*  Function description
*    Copy recorder statistics for all core log sources.
*
*  Parameters
*    stats  Destination statistics structure.
*
*  Return value
*    0   Statistics copied.
*   -1   Recorder is not initialized or destination is invalid.
*/
int CoreLogRecorder_GetStats(CoreLogRecorder_Stats_t *stats) {
    CoreLogRecorder_Source_t       *source;
    CoreLogRecorder_SourceStats_t  *source_stats;
    unsigned                        i;

    if (!_recorder_state.initialized || stats == NULL) {
        return -1;
    }

    memset(stats, 0, sizeof(CoreLogRecorder_Stats_t));

    for (i = 0u; i < CORE_LOG_RECORDER_NUM_SOURCES; i++) {
        source = &_recorder_state.sources[i];
        source_stats = (i == CORE_LOG_RECORDER_LINUX) ?
                       &stats->linux :
                       &stats->rtos;

        source_stats->enabled = source->enabled;
        source_stats->channel = source->channel;

        if (!source->enabled || !source->lock_initialized) {
            continue;
        }

        SYS_MutexLock(&source->lock);
        source_stats->bytes_recorded = source->bytes_recorded;
        source_stats->bytes_consumed = source->bytes_consumed;
        source_stats->consumer_undelivered_bytes = source->consumer_undelivered_bytes;
        source_stats->consumer_capacity_failure_count = source->consumer_capacity_failure_count;
        source_stats->consumer_registered = source->consumer_registered;
        source_stats->file_error = source->file_error;
        SYS_MutexUnlock(&source->lock);
    }

    stats->fatal_error = _Recorder_HasFatalError();
    return 0;
}
