/*********************************************************************
*                             CineLogic                              *
*                      RTT Trace and Debug Bridge                    *
**********************************************************************
----------------------------------------------------------------------
File    : CoreLogRecorderRegistry.c
Purpose : Core log recorder source descriptors
---------------------------END-OF-HEADER------------------------------
*/

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include "CoreLogRecorder_internal.h"

/*********************************************************************
*
*       Internal functions
*
**********************************************************************
*/

/*********************************************************************
*
*       _Recorder_FindSource()
*
*  Function description
*    Return the source state for an RTT channel.
*/
CoreLogRecorder_Source_t *_Recorder_FindSource(unsigned channel) {
    CoreLogRecorder_Source_t *source;

    source = &_recorder_state.sources[CORE_LOG_RECORDER_LINUX];
    if (source->enabled && source->channel == channel) {
        return source;
    }

    source = &_recorder_state.sources[CORE_LOG_RECORDER_RTOS];
    if (source->enabled && source->channel == channel) {
        return source;
    }

    return NULL;
}

/*********************************************************************
*
*       _Recorder_SourceIsEnabled()
*
*  Function description
*    Return whether a source participates in recording.
*/
bool _Recorder_SourceIsEnabled(CoreLogRecorder_SourceId_t source_id) {
    if (source_id >= CORE_LOG_RECORDER_NUM_SOURCES) {
        return false;
    }

    return _recorder_state.sources[source_id].enabled;
}

/*********************************************************************
*
*       _Recorder_DestroySourceLocks()
*
*  Function description
*    Destroy per-source locks.
*/
void _Recorder_DestroySourceLocks(void) {
    unsigned i;

    for (i = 0u; i < CORE_LOG_RECORDER_NUM_SOURCES; i++) {
        CoreLogRecorder_Source_t *source = &_recorder_state.sources[i];

        if (source->enabled && source->lock_initialized) {
            SYS_MutexDestroy(&source->lock);
            source->lock_initialized = false;
        }
    }
}

/*********************************************************************
*
*       _Recorder_InitSource()
*
*  Function description
*    Initialize one source descriptor and create its log file.
*/
int _Recorder_InitSource(CoreLogRecorder_SourceId_t source_id,
                         unsigned channel,
                         bool enabled,
                         const char *name,
                         const char *prefix) {
    CoreLogRecorder_Source_t *source;

    if (source_id >= CORE_LOG_RECORDER_NUM_SOURCES ||
        name == NULL) {
        return -1;
    }

    source = &_recorder_state.sources[source_id];
    source->channel = channel;
    source->name    = name;
    source->enabled = enabled;
    LOG_TextCleanStateInit(&source->file_clean_state);

    if (!enabled) {
        return 0;
    }
    if (prefix == NULL) {
        return -1;
    }

    if (SYS_MutexInit(&source->lock) != 0) {
        return -1;
    }
    source->lock_initialized = true;

    if (_Recorder_AllocateConsumerQueue(source) != 0) {
        return -1;
    }

    source->file = LOG_CreateTimestampedFile(prefix);
    if (source->file == NULL) {
        return -1;
    }
    source->last_flush_time_us = SYS_GetMonotonicTimeUs();

    return 0;
}

/*************************** End of file ****************************/
