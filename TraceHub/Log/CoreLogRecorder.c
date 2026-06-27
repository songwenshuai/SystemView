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
File    : CoreLogRecorder.c
Purpose : Core log recorder implementation
---------------------------END-OF-HEADER------------------------------
*/

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "CoreLogRecorder.h"
#include "Log.h"
#include "RTTBridge.h"
#include "SYS.h"

/*********************************************************************
*
*       Defines
*
**********************************************************************
*/

#define CORE_LOG_RECORDER_BUFFER_SIZE        CORE_LOG_RECORDER_MIN_QUEUE_SIZE
#define CORE_LOG_RECORDER_FLUSH_INTERVAL_US  100000u

/*********************************************************************
*
*       Types
*
**********************************************************************
*/

typedef enum {
    CORE_LOG_RECORDER_LINUX = 0,
    CORE_LOG_RECORDER_RTOS,
    CORE_LOG_RECORDER_NUM_SOURCES
} CoreLogRecorder_SourceId_t;

/*********************************************************************
*
*       CoreLogRecorder_Source_t
*
*  Description
*    Per-source file and optional consumer stream state.
*/
typedef struct {
    bool         enabled;
    unsigned     channel;
    const char  *name;
    FILE        *file;
    LOG_TextCleanState_t file_clean_state;
    bool         lock_initialized;
    SYS_Mutex    lock;
    bool         consumer_registered;
    bool         consumer_ever_registered;
    bool         seen;
    bool         file_error;
    bool         file_error_logged;
    bool         flush_pending;
    uint64_t     last_flush_time_us;
    uint64_t     bytes_recorded;
    uint64_t     bytes_consumed;
    uint64_t     consumer_undelivered_bytes;
    uint64_t     pending_undelivered_bytes;
    unsigned     consumer_capacity_failure_count;
    bool         queue_capacity_failure_pending;
    size_t       queue_size;
    size_t       queue_read;
    size_t       queue_used;
    char        *queue;
} CoreLogRecorder_Source_t;

/*********************************************************************
*
*       CoreLogRecorder_State_t
*
*  Description
*    Runtime state of the core log recorder.
*/
typedef struct {
    CoreLogRecorder_Config_t config;
    bool                     initialized;
    bool                     running;
    bool                     fatal_error;
    bool                     lock_initialized;
    SYS_Mutex                lock;
    SYS_Thread               linux_thread;
    SYS_Thread               rtos_thread;
    bool                     linux_thread_started;
    bool                     rtos_thread_started;
    CoreLogRecorder_Source_t sources[CORE_LOG_RECORDER_NUM_SOURCES];
} CoreLogRecorder_State_t;

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

static CoreLogRecorder_State_t _recorder_state;

/*********************************************************************
*
*       Static functions
*
**********************************************************************
*/

/*********************************************************************
*
*       _Recorder_IsRunning()
*
*  Function description
*    Read running state through the module lock.
*/
static bool _Recorder_IsRunning(void) {
    bool running;

    if (!_recorder_state.lock_initialized) {
        return _recorder_state.running;
    }

    SYS_MutexLock(&_recorder_state.lock);
    running = _recorder_state.running;
    SYS_MutexUnlock(&_recorder_state.lock);
    return running;
}

/*********************************************************************
*
*       _Recorder_SetRunning()
*
*  Function description
*    Update running state through the module lock.
*/
static void _Recorder_SetRunning(bool running) {
    if (!_recorder_state.lock_initialized) {
        _recorder_state.running = running;
        return;
    }

    SYS_MutexLock(&_recorder_state.lock);
    _recorder_state.running = running;
    SYS_MutexUnlock(&_recorder_state.lock);
}

/*********************************************************************
*
*       _Recorder_SetFatalError()
*
*  Function description
*    Mark recorder fatal state and stop recorder threads.
*/
static void _Recorder_SetFatalError(void) {
    if (!_recorder_state.lock_initialized) {
        _recorder_state.fatal_error = true;
        _recorder_state.running = false;
        return;
    }

    SYS_MutexLock(&_recorder_state.lock);
    _recorder_state.fatal_error = true;
    _recorder_state.running = false;
    SYS_MutexUnlock(&_recorder_state.lock);
}

/*********************************************************************
*
*       _Recorder_HasFatalError()
*
*  Function description
*    Read fatal error state through the module lock.
*/
static bool _Recorder_HasFatalError(void) {
    bool fatal_error;

    if (!_recorder_state.lock_initialized) {
        return _recorder_state.fatal_error;
    }

    SYS_MutexLock(&_recorder_state.lock);
    fatal_error = _recorder_state.fatal_error;
    SYS_MutexUnlock(&_recorder_state.lock);
    return fatal_error;
}

/*********************************************************************
*
*       _Recorder_GetPollInterval()
*
*  Function description
*    Return the configured poll interval or a documented default.
*/
static unsigned _Recorder_GetPollInterval(void) {
    if (_recorder_state.config.poll_interval_ms == 0u) {
        return RTT_BRIDGE_DEFAULT_POLL_INTERVAL_MS;
    }

    return _recorder_state.config.poll_interval_ms;
}

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
static bool _Recorder_SourceIsEnabled(CoreLogRecorder_SourceId_t source_id) {
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
static void _Recorder_RecordBytes(CoreLogRecorder_SourceId_t source_id,
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
*       _Recorder_CheckSourceChannel()
*
*  Function description
*    Verify that a source RTT up-buffer exists before recording starts.
*/
static int _Recorder_CheckSourceChannel(CoreLogRecorder_SourceId_t source_id) {
    CoreLogRecorder_Source_t *source;

    if (source_id >= CORE_LOG_RECORDER_NUM_SOURCES) {
        return -1;
    }

    source = &_recorder_state.sources[source_id];
    if (!source->enabled) {
        return 0;
    }

    if (RTTBridge_CheckUpBufferChannel(source->channel) != 0) {
        fprintf(stderr,
                "[CoreLogRecorder] %s RTT up-buffer channel %u is not configured\n",
                source->name,
                source->channel);
        return -1;
    }

    return 0;
}

/*********************************************************************
*
*       _Recorder_CheckSourceChannels()
*
*  Function description
*    Verify all core RTT up-buffer channels before recorder threads start.
*/
static int _Recorder_CheckSourceChannels(void) {
    int      result;
    unsigned i;

    result = 0;
    for (i = 0u; i < CORE_LOG_RECORDER_NUM_SOURCES; i++) {
        if (!_Recorder_SourceIsEnabled((CoreLogRecorder_SourceId_t)i)) {
            continue;
        }
        if (_Recorder_CheckSourceChannel((CoreLogRecorder_SourceId_t)i) != 0) {
            result = -1;
        }
    }

    return result;
}

/*********************************************************************
*
*       _Recorder_GetRecoveryTimeout()
*
*  Function description
*    Return the RTT control block recovery timeout in milliseconds.
*/
static unsigned _Recorder_GetRecoveryTimeout(void) {
    const RTTBridge_State_t *bridge_state;

    bridge_state = RTTBridge_GetState();
    if (bridge_state == NULL) {
        return 0u;
    }

    return bridge_state->config.rtt_search_timeout_ms;
}

/*********************************************************************
*
*       _Recorder_WaitForSourceRecovery()
*
*  Function description
*    Wait until a source RTT up-buffer is valid again after target reset.
*/
static int _Recorder_WaitForSourceRecovery(CoreLogRecorder_SourceId_t source_id) {
    CoreLogRecorder_Source_t *source;
    unsigned                  retry_interval_ms;
    unsigned                  timeout_ms;
    unsigned                  elapsed_ms;
    unsigned                  sleep_ms;
    bool                      reported;

    if (source_id >= CORE_LOG_RECORDER_NUM_SOURCES) {
        _Recorder_SetFatalError();
        return -1;
    }

    source = &_recorder_state.sources[source_id];
    if (!source->enabled) {
        return -1;
    }

    retry_interval_ms = _Recorder_GetPollInterval();
    if (retry_interval_ms == 0u) {
        _Recorder_SetFatalError();
        return -1;
    }

    timeout_ms = _Recorder_GetRecoveryTimeout();
    elapsed_ms = 0u;
    reported = false;

    while (_Recorder_IsRunning() && RTTBridge_IsRunning()) {
        if (RTTBridge_CheckUpBufferChannel(source->channel) == 0) {
            if (reported) {
                fprintf(stderr,
                        "[CoreLogRecorder] %s RTT up-buffer channel %u recovered\n",
                        source->name,
                        source->channel);
            }
            return 0;
        }

        if (!reported) {
            fprintf(stderr,
                    "[CoreLogRecorder] %s RTT up-buffer channel %u unavailable, waiting for recovery\n",
                    source->name,
                    source->channel);
            reported = true;
        }

        if ((timeout_ms > 0u) && (elapsed_ms >= timeout_ms)) {
            fprintf(stderr,
                    "[CoreLogRecorder] %s RTT up-buffer channel %u did not recover within %u ms\n",
                    source->name,
                    source->channel,
                    timeout_ms);
            _Recorder_SetFatalError();
            return -1;
        }

        sleep_ms = retry_interval_ms;
        if (timeout_ms > 0u) {
            unsigned remaining_ms;

            remaining_ms = timeout_ms - elapsed_ms;
            if (sleep_ms > remaining_ms) {
                sleep_ms = remaining_ms;
            }
        }

        SYS_Sleep(sleep_ms);
        if (sleep_ms > (UINT_MAX - elapsed_ms)) {
            elapsed_ms = UINT_MAX;
        } else {
            elapsed_ms += sleep_ms;
        }
    }

    return -1;
}

/*********************************************************************
*
*       _Recorder_DrainSource()
*
*  Function description
*    Read one core RTT up-buffer and record every byte read.
*/
static void _Recorder_DrainSource(CoreLogRecorder_SourceId_t source_id) {
    char     buffer[CORE_LOG_RECORDER_BUFFER_SIZE];
    unsigned channel;
    int      num_bytes;

    channel = _recorder_state.sources[source_id].channel;
    if (!_Recorder_SourceIsEnabled(source_id)) {
        return;
    }

    while (_Recorder_IsRunning() && RTTBridge_IsRunning()) {
        num_bytes = RTTBridge_ReadUpBufferNoLock(channel, buffer, sizeof(buffer));
        if (num_bytes < 0) {
            if (_Recorder_WaitForSourceRecovery(source_id) != 0) {
                break;
            }
            continue;
        }
        if (num_bytes == 0) {
            SYS_Sleep(_Recorder_GetPollInterval());
            continue;
        }
        _Recorder_RecordBytes(source_id, buffer, (unsigned)num_bytes);
    }
}

/*********************************************************************
*
*       _Recorder_LinuxThread()
*
*  Function description
*    Thread entry for draining Linux core logs.
*
*  Parameters
*    arg  Unused thread context.
*/
static void _Recorder_LinuxThread(void *arg) {
    (void)arg;

    _Recorder_DrainSource(CORE_LOG_RECORDER_LINUX);
}

/*********************************************************************
*
*       _Recorder_RTOSThread()
*
*  Function description
*    Thread entry for draining RTOS core logs.
*
*  Parameters
*    arg  Unused thread context.
*/
static void _Recorder_RTOSThread(void *arg) {
    (void)arg;

    _Recorder_DrainSource(CORE_LOG_RECORDER_RTOS);
}

/*********************************************************************
*
*       _Recorder_FlushAllSources()
*
*  Function description
*    Flush all owned log files.
*/
static void _Recorder_FlushAllSources(void) {
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
static void _Recorder_CloseFiles(void) {
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
*       _Recorder_ReportStats()
*
*  Function description
*    Print per-core consumer queue delivery failure statistics.
*/
static void _Recorder_ReportStats(void) {
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
*       _Recorder_DestroySourceLocks()
*
*  Function description
*    Destroy per-source locks.
*/
static void _Recorder_DestroySourceLocks(void) {
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
static void _Recorder_FreeSourceQueues(void) {
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
static int _Recorder_InitSource(CoreLogRecorder_SourceId_t source_id,
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
*       Public functions
*
**********************************************************************
*/

/*********************************************************************
*
*       CoreLogRecorder_Init()
*
*  Function description
*    Initialize the core log recorder and prepare enabled source output files.
*
*  Parameters
*    config  Recorder configuration.
*
*  Return value
*    0   Success.
*   -1   Invalid configuration or recorder state.
*   -2   Failed to initialize an enabled source.
*/
int CoreLogRecorder_Init(const CoreLogRecorder_Config_t *config) {
    const char *linux_prefix;
    const char *rtos_prefix;

    if (config == NULL) {
        return -1;
    }
    if (!config->linux_enabled && !config->rtos_enabled) {
        return -1;
    }
    if (config->linux_enabled &&
        config->rtos_enabled &&
        config->linux_channel == config->rtos_channel) {
        return -1;
    }
    if (_Recorder_IsRunning()) {
        return -1;
    }
    if (_recorder_state.initialized) {
        CoreLogRecorder_Cleanup();
    }
    if (CORE_LOG_RECORDER_DEFAULT_QUEUE_SIZE < CORE_LOG_RECORDER_MIN_QUEUE_SIZE) {
        return -1;
    }

    memset(&_recorder_state, 0, sizeof(_recorder_state));
    memcpy(&_recorder_state.config, config, sizeof(CoreLogRecorder_Config_t));

    if (SYS_MutexInit(&_recorder_state.lock) != 0) {
        return -1;
    }
    _recorder_state.lock_initialized = true;

    linux_prefix = (config->linux_prefix != NULL) ? config->linux_prefix : "linux";
    rtos_prefix  = (config->rtos_prefix  != NULL) ? config->rtos_prefix  : "rtos";

    if (_Recorder_InitSource(CORE_LOG_RECORDER_LINUX,
                             config->linux_channel,
                             config->linux_enabled,
                             "Linux",
                             linux_prefix) != 0) {
        CoreLogRecorder_Cleanup();
        return -2;
    }
    if (_Recorder_InitSource(CORE_LOG_RECORDER_RTOS,
                             config->rtos_channel,
                             config->rtos_enabled,
                             "RTOS",
                             rtos_prefix) != 0) {
        CoreLogRecorder_Cleanup();
        return -2;
    }

    _recorder_state.initialized = true;
    return 0;
}

/*********************************************************************
*
*       CoreLogRecorder_Start()
*
*  Function description
*    Start recorder threads for enabled core log sources.
*
*  Return value
*    0   Success.
*   -1   Recorder is not initialized, has failed, or is already running.
*   -2   Failed to create a recorder thread.
*   -3   RTT source channel validation failed.
*/
int CoreLogRecorder_Start(void) {
    int result;

    if (!_recorder_state.initialized || _Recorder_HasFatalError()) {
        return -1;
    }
    if (_Recorder_IsRunning() ||
        _recorder_state.linux_thread_started ||
        _recorder_state.rtos_thread_started) {
        return -1;
    }
    if (_Recorder_CheckSourceChannels() != 0) {
        _Recorder_SetFatalError();
        return -3;
    }

    _Recorder_SetRunning(true);

    if (_Recorder_SourceIsEnabled(CORE_LOG_RECORDER_LINUX)) {
        result = SYS_createThread(_Recorder_LinuxThread, NULL, &_recorder_state.linux_thread);
        if (result < 0) {
            _Recorder_SetRunning(false);
            return -2;
        }
        _recorder_state.linux_thread_started = true;
    }

    if (_Recorder_SourceIsEnabled(CORE_LOG_RECORDER_RTOS)) {
        result = SYS_createThread(_Recorder_RTOSThread, NULL, &_recorder_state.rtos_thread);
        if (result < 0) {
            _Recorder_SetRunning(false);
            if (_recorder_state.linux_thread_started) {
                SYS_WaitThreadTerm(_recorder_state.linux_thread);
                _recorder_state.linux_thread_started = false;
            }
            return -2;
        }
        _recorder_state.rtos_thread_started = true;
    }

    return 0;
}

/*********************************************************************
*
*       CoreLogRecorder_Stop()
*
*  Function description
*    Stop recorder threads, flush pending file output, and report
*    recorder statistics.
*/
void CoreLogRecorder_Stop(void) {
    _Recorder_SetRunning(false);

    if (_recorder_state.linux_thread_started) {
        SYS_WaitThreadTerm(_recorder_state.linux_thread);
        _recorder_state.linux_thread_started = false;
    }
    if (_recorder_state.rtos_thread_started) {
        SYS_WaitThreadTerm(_recorder_state.rtos_thread);
        _recorder_state.rtos_thread_started = false;
    }

    _Recorder_FlushAllSources();
    _Recorder_ReportStats();
    _Recorder_CloseFiles();
}

/*********************************************************************
*
*       CoreLogRecorder_Cleanup()
*
*  Function description
*    Stop recording when needed and release recorder resources.
*/
void CoreLogRecorder_Cleanup(void) {
    if (_Recorder_IsRunning() ||
        _recorder_state.linux_thread_started ||
        _recorder_state.rtos_thread_started) {
        CoreLogRecorder_Stop();
    }

    _Recorder_CloseFiles();
    _Recorder_DestroySourceLocks();
    _Recorder_FreeSourceQueues();

    if (_recorder_state.lock_initialized) {
        SYS_MutexDestroy(&_recorder_state.lock);
        _recorder_state.lock_initialized = false;
    }

    memset(&_recorder_state, 0, sizeof(_recorder_state));
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

/*********************************************************************
*
*       CoreLogRecorder_HasFatalError()
*
*  Function description
*    Check whether the recorder has entered a fatal error state.
*
*  Return value
*    true   Fatal error occurred.
*    false  No fatal error recorded.
*/
bool CoreLogRecorder_HasFatalError(void) {
    return _Recorder_HasFatalError();
}

/*********************************************************************
*
*       CoreLogRecorder_IsRunning()
*
*  Function description
*    Check whether core log recorder threads are running.
*
*  Return value
*    true   Recorder is running.
*    false  Recorder is stopped.
*/
bool CoreLogRecorder_IsRunning(void) {
    return _Recorder_IsRunning();
}

/*************************** End of file ****************************/
