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
File    : CoreLogRecorder.h
Purpose : Core log recorder for RTT text sources
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
*    Per-source recorder statistics.
*    bytes_recorded counts clean text bytes written to the source log file.
*    bytes_consumed counts raw bytes delivered to the registered consumer.
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
*    Recorder statistics for both text log sources.
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
