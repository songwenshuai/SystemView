/*********************************************************************
*                             CineLogic                              *
*                      RTT Trace and Debug Bridge                    *
**********************************************************************
----------------------------------------------------------------------
File    : CoreLogRecorderFile.c
Purpose : Core log recorder file persistence
---------------------------END-OF-HEADER------------------------------
*/

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include <errno.h>
#include <stdio.h>

#include "CoreLogRecorder_internal.h"

/*********************************************************************
*
*       Internal functions
*
**********************************************************************
*/

/*********************************************************************
*
*       _Recorder_ReportFileErrorLocked()
*
*  Function description
*    Record a source file failure. Caller must hold source lock.
*/
void _Recorder_ReportFileErrorLocked(CoreLogRecorder_Source_t *source, const char *operation) {
    if (source == NULL) {
        return;
    }
    if (operation == NULL) {
        operation = "operation";
    }

    source->file_error = true;
    if (!source->file_error_logged) {
        fprintf(stderr,
                "[CoreLogRecorder] %s file %s failed\n",
                source->name, operation);
        source->file_error_logged = true;
    }
    _Recorder_SetFatalError();
}

/*********************************************************************
*
*       _Recorder_FlushSourceLocked()
*
*  Function description
*    Flush pending file data. Caller must hold source lock.
*/
int _Recorder_FlushSourceLocked(CoreLogRecorder_Source_t *source) {
    if (source == NULL || source->file == NULL || !source->flush_pending) {
        return 0;
    }

    if (fflush(source->file) != 0) {
        _Recorder_ReportFileErrorLocked(source, "flush");
        return -1;
    }

    source->flush_pending = false;
    source->last_flush_time_us = SYS_GetMonotonicTimeUs();
    return 0;
}

/*********************************************************************
*
*       _Recorder_RecordBytes()
*
*  Function description
*    Record clean text to the owned file and publish bytes to consumers.
*/
void _Recorder_RecordBytes(CoreLogRecorder_SourceId_t source_id,
                           const char *data, unsigned num_bytes) {
    CoreLogRecorder_Source_t *source;
    size_t                    clean_len;
    uint64_t                  now_us;

    if (source_id >= CORE_LOG_RECORDER_NUM_SOURCES ||
        data == NULL ||
        num_bytes == 0u) {
        return;
    }

    source = &_recorder_state.sources[source_id];
    if (!source->enabled) {
        return;
    }

    SYS_MutexLock(&source->lock);

    if (source->file == NULL || source->file_error) {
        _Recorder_ReportFileErrorLocked(source, "write");
        SYS_MutexUnlock(&source->lock);
        return;
    }

    clean_len = 0u;
    if (LOG_WriteCleanTextToFileEx(source->file,
                                   data,
                                   num_bytes,
                                   &source->file_clean_state,
                                   &clean_len) != 0) {
        _Recorder_ReportFileErrorLocked(source, "write");
        SYS_MutexUnlock(&source->lock);
        return;
    }

    source->flush_pending = true;
    source->bytes_recorded += clean_len;

    now_us = SYS_GetMonotonicTimeUs();
    if ((source->last_flush_time_us == 0u) ||
        ((now_us - source->last_flush_time_us) >= CORE_LOG_RECORDER_FLUSH_INTERVAL_US)) {
        if (_Recorder_FlushSourceLocked(source) != 0) {
            SYS_MutexUnlock(&source->lock);
            return;
        }
    }

    if (!source->seen) {
        fprintf(stderr,
                "[CoreLogRecorder] %s log detected on RTT channel %u\n",
                source->name, source->channel);
        source->seen = true;
    }

    if (!source->consumer_ever_registered || source->consumer_registered) {
        if (_Recorder_QueueBytesLocked(source, data, num_bytes) != 0) {
            SYS_MutexUnlock(&source->lock);
            return;
        }
    }

    SYS_MutexUnlock(&source->lock);
}

/*********************************************************************
*
*       _Recorder_FlushAllSources()
*
*  Function description
*    Flush all owned log files.
*/
void _Recorder_FlushAllSources(void) {
    unsigned i;

    for (i = 0u; i < CORE_LOG_RECORDER_NUM_SOURCES; i++) {
        CoreLogRecorder_Source_t *source = &_recorder_state.sources[i];

        if (source->enabled && source->lock_initialized) {
            SYS_MutexLock(&source->lock);
            (void)_Recorder_FlushSourceLocked(source);
            SYS_MutexUnlock(&source->lock);
        }
    }
}

/*********************************************************************
*
*       _Recorder_CloseFiles()
*
*  Function description
*    Close all owned log files.
*/
void _Recorder_CloseFiles(void) {
    unsigned i;

    for (i = 0u; i < CORE_LOG_RECORDER_NUM_SOURCES; i++) {
        CoreLogRecorder_Source_t *source = &_recorder_state.sources[i];
        FILE                     *file;

        if (source->enabled && source->lock_initialized) {
            SYS_MutexLock(&source->lock);
        }
        if (source->file != NULL) {
            file = source->file;
            if (source->flush_pending) {
                (void)_Recorder_FlushSourceLocked(source);
            }
            source->file = NULL;
            errno = 0;
            if (fclose(file) != 0) {
                _Recorder_ReportFileErrorLocked(source, "close");
            }
        }
        if (source->enabled && source->lock_initialized) {
            SYS_MutexUnlock(&source->lock);
        }
    }
}

/*************************** End of file ****************************/
