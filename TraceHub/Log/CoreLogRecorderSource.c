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
File    : CoreLogRecorderSource.c
Purpose : Core log recorder source files and consumer queues
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
*       _Recorder_FindSource()
*
*  Function description
*    Return the source state for an RTT channel.
*/
static CoreLogRecorder_Source_t *_Recorder_FindSource(unsigned channel) {
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
*       _Recorder_ReportFileErrorLocked()
*
*  Function description
*    Record a source file failure. Caller must hold source lock.
*/
static void _Recorder_ReportFileErrorLocked(CoreLogRecorder_Source_t *source, const char *operation) {
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
static int _Recorder_FlushSourceLocked(CoreLogRecorder_Source_t *source) {
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
*       _Recorder_QueueBytesLocked()
*
*  Function description
*    Append recorded bytes to the per-core consumer queue.
*
*  Return value
*    0   Success
*   -1   Queue capacity would be exceeded
*/
static int _Recorder_QueueBytesLocked(CoreLogRecorder_Source_t *source,
                                      const char *data, unsigned num_bytes) {
    size_t free_space;
    size_t write_pos;
    size_t first_chunk;

    if (source == NULL || data == NULL || num_bytes == 0u) {
        return 0;
    }
    if (source->queue == NULL || source->queue_size == 0u) {
        _Recorder_SetFatalError();
        return -1;
    }

    free_space = source->queue_size - source->queue_used;
    if ((size_t)num_bytes > free_space) {
        source->consumer_undelivered_bytes += num_bytes;
        source->pending_undelivered_bytes += num_bytes;
        source->consumer_capacity_failure_count++;
        source->queue_capacity_failure_pending = true;
        fprintf(stderr,
                "[CoreLogRecorder] %s consumer queue capacity exceeded: "
                "%u bytes cannot be delivered without loss; capacity=%zu; stopping recorder\n",
                source->name,
                num_bytes,
                source->queue_size);
        _Recorder_SetFatalError();
        return -1;
    }

    write_pos = (source->queue_read + source->queue_used) %
                source->queue_size;
    first_chunk = source->queue_size - write_pos;
    if (first_chunk > (size_t)num_bytes) {
        first_chunk = (size_t)num_bytes;
    }

    memcpy(&source->queue[write_pos], data, first_chunk);
    if (first_chunk < (size_t)num_bytes) {
        memcpy(&source->queue[0], data + first_chunk, num_bytes - first_chunk);
    }
    source->queue_used += num_bytes;
    return 0;
}


/*********************************************************************
*
*       _Recorder_ClearConsumerQueueLocked()
*
*  Function description
*    Drop all bytes owned by the transient consumer stream.
*/
static void _Recorder_ClearConsumerQueueLocked(CoreLogRecorder_Source_t *source) {
    if (source == NULL) {
        return;
    }

    source->queue_read = 0u;
    source->queue_used = 0u;
    source->pending_undelivered_bytes = 0u;
    source->queue_capacity_failure_pending = false;
}


/*********************************************************************
*
*       _Recorder_AllocateConsumerQueue()
*
*  Function description
*    Allocate the per-source consumer queue.
*/
static int _Recorder_AllocateConsumerQueue(CoreLogRecorder_Source_t *source) {
    char   *queue;
    size_t  queue_size;

    if (source == NULL || source->queue != NULL || source->queue_size != 0u) {
        return -1;
    }

    queue_size = CORE_LOG_RECORDER_DEFAULT_QUEUE_SIZE;
    if (queue_size < CORE_LOG_RECORDER_MIN_QUEUE_SIZE) {
        return -1;
    }

    queue = (char *)malloc(queue_size);
    if (queue == NULL) {
        return -1;
    }

    source->queue = queue;
    source->queue_size = queue_size;
    _Recorder_ClearConsumerQueueLocked(source);
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
*       _Recorder_FreeSourceQueues()
*
*  Function description
*    Free per-source consumer queues.
*/
void _Recorder_FreeSourceQueues(void) {
    unsigned i;

    for (i = 0u; i < CORE_LOG_RECORDER_NUM_SOURCES; i++) {
        CoreLogRecorder_Source_t *source = &_recorder_state.sources[i];

        free(source->queue);
        source->queue = NULL;
        source->queue_size = 0u;
        source->queue_read = 0u;
        source->queue_used = 0u;
        source->consumer_registered = false;
        source->consumer_ever_registered = false;
        source->pending_undelivered_bytes = 0u;
        source->queue_capacity_failure_pending = false;
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


/*********************************************************************
*
*       CoreLogRecorder_RegisterConsumer()
*
*  Function description
*    Register a consumer for a core log channel.
*
*  Parameters
*    channel  RTT up-buffer channel used by the source.
*
*  Return value
*    0   Consumer registered.
*   -1   Recorder or channel is invalid.
*   -2   Consumer is already registered.
*/
int CoreLogRecorder_RegisterConsumer(unsigned channel) {
    CoreLogRecorder_Source_t *source;

    if (!_recorder_state.initialized) {
        return -1;
    }

    source = _Recorder_FindSource(channel);
    if (source == NULL || !source->lock_initialized) {
        return -1;
    }

    SYS_MutexLock(&source->lock);
    if (source->consumer_registered) {
        SYS_MutexUnlock(&source->lock);
        return -2;
    }
    if (source->queue == NULL || source->queue_size == 0u) {
        SYS_MutexUnlock(&source->lock);
        return -1;
    }
    source->consumer_registered = true;
    source->consumer_ever_registered = true;
    SYS_MutexUnlock(&source->lock);

    return 0;
}


/*********************************************************************
*
*       CoreLogRecorder_UnregisterConsumer()
*
*  Function description
*    Unregister the consumer for a core log channel.
*
*  Parameters
*    channel  RTT up-buffer channel used by the source.
*/
void CoreLogRecorder_UnregisterConsumer(unsigned channel) {
    CoreLogRecorder_Source_t *source;

    if (!_recorder_state.initialized) {
        return;
    }

    source = _Recorder_FindSource(channel);
    if (source == NULL || !source->lock_initialized) {
        return;
    }

    SYS_MutexLock(&source->lock);
    source->consumer_registered = false;
    _Recorder_ClearConsumerQueueLocked(source);
    SYS_MutexUnlock(&source->lock);
}


/*********************************************************************
*
*       CoreLogRecorder_ReadChannel()
*
*  Function description
*    Read queued core log bytes for a registered consumer.
*
*  Parameters
*    channel      RTT up-buffer channel used by the source.
*    buffer       Destination buffer.
*    buffer_size  Destination buffer size in bytes.
*
*  Return value
*    >= 0  Number of bytes read.
*    -1    Recorder, channel, or buffer is invalid.
*    -2    No consumer is registered for the channel.
*    -3    Consumer queue capacity failure was detected.
*/
int CoreLogRecorder_ReadChannel(unsigned channel, void *buffer, size_t buffer_size) {
    CoreLogRecorder_Source_t *source;
    size_t                    num_bytes;
    size_t                    first_chunk;
    bool                      queue_capacity_failure_pending;
    uint64_t                  pending_undelivered_bytes;
    const char               *source_name;

    if (!_recorder_state.initialized ||
        buffer == NULL ||
        buffer_size == 0u ||
        buffer_size > (size_t)INT_MAX) {
        return -1;
    }

    source = _Recorder_FindSource(channel);
    if (source == NULL || !source->lock_initialized) {
        return -1;
    }

    SYS_MutexLock(&source->lock);

    if (!source->consumer_registered) {
        SYS_MutexUnlock(&source->lock);
        return -2;
    }
    if (source->queue == NULL || source->queue_size == 0u) {
        SYS_MutexUnlock(&source->lock);
        return -1;
    }

    num_bytes = source->queue_used;
    if (num_bytes > buffer_size) {
        num_bytes = buffer_size;
    }

    queue_capacity_failure_pending = source->queue_capacity_failure_pending;
    pending_undelivered_bytes = source->pending_undelivered_bytes;
    source->queue_capacity_failure_pending = false;
    source->pending_undelivered_bytes = 0u;
    source_name = source->name;

    if (queue_capacity_failure_pending) {
        SYS_MutexUnlock(&source->lock);
        fprintf(stderr,
                "[CoreLogRecorder] %s consumer queue capacity failure: "
                "%llu bytes were not delivered to the consumer\n",
                source_name,
                (unsigned long long)pending_undelivered_bytes);
        return -3;
    }

    if (num_bytes == 0u) {
        SYS_MutexUnlock(&source->lock);
        return 0;
    }

    first_chunk = source->queue_size - source->queue_read;
    if (first_chunk > num_bytes) {
        first_chunk = num_bytes;
    }

    memcpy(buffer, &source->queue[source->queue_read], first_chunk);
    if (first_chunk < num_bytes) {
        memcpy((char *)buffer + first_chunk,
               &source->queue[0],
               num_bytes - first_chunk);
    }

    source->queue_read = (source->queue_read + num_bytes) %
                         source->queue_size;
    source->queue_used -= num_bytes;
    if (source->queue_used == 0u) {
        source->queue_read = 0u;
    }
    source->bytes_consumed += num_bytes;

    SYS_MutexUnlock(&source->lock);

    return (int)num_bytes;
}


/*********************************************************************
*
*       CoreLogRecorder_IsCoreChannel()
*
*  Function description
*    Check whether a channel belongs to an initialized core log source.
*
*  Parameters
*    channel  RTT up-buffer channel to check.
*
*  Return value
*    true   Channel belongs to a core log source.
*    false  Channel is not a core log source.
*/
bool CoreLogRecorder_IsCoreChannel(unsigned channel) {
    if (!_recorder_state.initialized) {
        return false;
    }

    return (_Recorder_FindSource(channel) != NULL);
}
