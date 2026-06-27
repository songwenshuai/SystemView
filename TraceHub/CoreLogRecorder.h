/*********************************************************************
*                                                                    *
*                Copyright (C) 2023 xrTest Inc.                      *
*                      All rights reserved                           *
*                                                                    *
**********************************************************************
----------------------------------------------------------------------
File    : CoreLogRecorder.h
Purpose : Core log recorder for Linux and RTOS RTT up-buffers
Author  : songwenshuai <songwenshuai@gmail.com>
---------------------------END-OF-HEADER------------------------------
*/

#ifndef TRACEHUB_CORELOGRECORDER_H
#define TRACEHUB_CORELOGRECORDER_H

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#if defined(__cplusplus)
extern "C" {
#endif

/*********************************************************************
*
*       Defines
*
**********************************************************************
*/

/*********************************************************************
*
*       CORE_LOG_RECORDER_DEFAULT_QUEUE_SIZE
*  Default per-source consumer queue capacity.
*
*/
#ifndef CORE_LOG_RECORDER_DEFAULT_QUEUE_SIZE
  #define CORE_LOG_RECORDER_DEFAULT_QUEUE_SIZE  (1024u * 1024u)
#endif

/*********************************************************************
*
*       CORE_LOG_RECORDER_MIN_QUEUE_SIZE
*  Minimum per-source consumer queue capacity.
*
*/
#ifndef CORE_LOG_RECORDER_MIN_QUEUE_SIZE
  #define CORE_LOG_RECORDER_MIN_QUEUE_SIZE      8192u
#endif

/*********************************************************************
*
*       Types
*
**********************************************************************
*/

/*********************************************************************
*
*       CoreLogRecorder_Config_t
*
*  Description
*    Configuration for Linux and RTOS core log recording.
*/
typedef struct {
    unsigned     linux_channel;
    unsigned     rtos_channel;
    unsigned     poll_interval_ms;
    size_t       consumer_queue_size;
    const char  *linux_prefix;
    const char  *rtos_prefix;
    bool         linux_enabled;
    bool         rtos_enabled;
} CoreLogRecorder_Config_t;

/*********************************************************************
*
*       CoreLogRecorder_SourceStats_t
*
*  Description
*    Per-core recorder statistics.
*/
typedef struct {
    unsigned channel;
    uint64_t bytes_recorded;
    uint64_t bytes_consumed;
    uint64_t consumer_undelivered_bytes;
    unsigned consumer_capacity_failure_count;
    bool     enabled;
    bool     consumer_registered;
    bool     file_error;
} CoreLogRecorder_SourceStats_t;

/*********************************************************************
*
*       CoreLogRecorder_Stats_t
*
*  Description
*    Recorder statistics for both core log sources.
*/
typedef struct {
    CoreLogRecorder_SourceStats_t linux;
    CoreLogRecorder_SourceStats_t rtos;
    bool                          fatal_error;
} CoreLogRecorder_Stats_t;

/*********************************************************************
*
*       API functions
*
**********************************************************************
*/

int  CoreLogRecorder_Init              (const CoreLogRecorder_Config_t *config);
int  CoreLogRecorder_Start             (void);
void CoreLogRecorder_Stop              (void);
void CoreLogRecorder_Cleanup           (void);
int  CoreLogRecorder_RegisterConsumer  (unsigned channel);
void CoreLogRecorder_UnregisterConsumer(unsigned channel);
int  CoreLogRecorder_ReadChannel       (unsigned channel, void *buffer, size_t buffer_size);
bool CoreLogRecorder_IsCoreChannel     (unsigned channel);
int  CoreLogRecorder_GetStats          (CoreLogRecorder_Stats_t *stats);
bool CoreLogRecorder_HasFatalError     (void);
bool CoreLogRecorder_IsRunning         (void);

#if defined(__cplusplus)
}
#endif

#endif

/*************************** End of file ****************************/
