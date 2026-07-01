/*********************************************************************
*                             CineLogic                              *
*                      RTT Trace and Debug Bridge                    *
**********************************************************************
----------------------------------------------------------------------
File    : CoreLogRecorderConsumer.c
Purpose : Core log recorder consumer queues
---------------------------END-OF-HEADER------------------------------
*/

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "CoreLogRecorder_internal.h"

/*********************************************************************
*
*       Internal functions
*
**********************************************************************
*/

/*********************************************************************
*
*       _Recorder_QueueBytesLocked()
*
*  Function description
*    Append recorded bytes to the per-core consumer queue.
*
*  Return value
*    0   Success.
*   -1   Queue capacity would be exceeded.
*/
int _Recorder_QueueBytesLocked(CoreLogRecorder_Source_t *source,
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
void _Recorder_ClearConsumerQueueLocked(CoreLogRecorder_Source_t *source) {
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
int _Recorder_AllocateConsumerQueue(CoreLogRecorder_Source_t *source) {
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
*       Public functions
*
**********************************************************************
*/

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

/*************************** End of file ****************************/
