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
File    : RTTBridge.h
Purpose : RTT Bridge core state and configuration management
---------------------------END-OF-HEADER------------------------------
*/

#ifndef TRACEHUB_RTTBRIDGE_H            // Guard against multiple inclusion
#define TRACEHUB_RTTBRIDGE_H

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <signal.h>

#include "SYS.h"

#if defined(__cplusplus)         // Allow usage of this module from C++ files (disable name mangling)
extern "C" {     /* Make sure we have C-declarations in C++ programs */
#endif

/*********************************************************************
*
*       Defines, fixed
*
**********************************************************************
*/

/*********************************************************************
*
*       RTT_BRIDGE_DEFAULT_POLL_INTERVAL_MS
*  Default polling interval in milliseconds for RTT buffer checking.
*
*/
#ifndef RTT_BRIDGE_DEFAULT_POLL_INTERVAL_MS
  #define RTT_BRIDGE_DEFAULT_POLL_INTERVAL_MS    10
#endif

/*********************************************************************
*
*       RTT_BRIDGE_DEFAULT_TERMINAL_PORT
*  Default TCP port for Terminal service.
*
*/
#ifndef RTT_BRIDGE_DEFAULT_TERMINAL_PORT
  #define RTT_BRIDGE_DEFAULT_TERMINAL_PORT       19021
#endif

/*********************************************************************
*
*       RTT_BRIDGE_DEFAULT_SYSVIEW_PORT
*  Default TCP port for SystemView service.
*
*/
#ifndef RTT_BRIDGE_DEFAULT_SYSVIEW_PORT
  #define RTT_BRIDGE_DEFAULT_SYSVIEW_PORT        19111
#endif

/*********************************************************************
*
*       RTT_BRIDGE_DEFAULT_TERMINAL_CHANNEL
*  Default RTT channel for Terminal.
*
*/
#ifndef RTT_BRIDGE_DEFAULT_TERMINAL_CHANNEL
  #define RTT_BRIDGE_DEFAULT_TERMINAL_CHANNEL    0
#endif

/*********************************************************************
*
*       RTT_BRIDGE_DEFAULT_RTOS_CHANNEL
*  Default RTT channel for RTOS core logs.
*
*/
#ifndef RTT_BRIDGE_DEFAULT_RTOS_CHANNEL
  #define RTT_BRIDGE_DEFAULT_RTOS_CHANNEL        1
#endif

/*********************************************************************
*
*       RTT_BRIDGE_DEFAULT_SYSVIEW_CHANNEL
*  Default RTT channel for SystemView.
*
*/
#ifndef RTT_BRIDGE_DEFAULT_SYSVIEW_CHANNEL
  #define RTT_BRIDGE_DEFAULT_SYSVIEW_CHANNEL     2
#endif

/*********************************************************************
*
*       RTT_COMM_POLL_INTERVAL
*  Polling interval to check for send threshold or idle delay timeout.
*
*/
#ifndef RTT_COMM_POLL_INTERVAL
  #define RTT_COMM_POLL_INTERVAL                 2
#endif

/*********************************************************************
*
*       RTT_SEND_THRESHOLD
*  Buffer level that triggers immediate forwarding to the client.
*
*/
#ifndef RTT_SEND_THRESHOLD
  #define RTT_SEND_THRESHOLD                     512
#endif

/*********************************************************************
*
*       Types
*
**********************************************************************
*/

/*********************************************************************
*
*       RTTBridge_Config_t
*
*  Description
*    Configuration structure for RTT bridge initialization.
*
*  Fields
*    device_path        Path to SharedMem device (e.g., "/dev/shared_mem0")
*    rtt_address        Backend address of RTT region
*    rtt_region_size    RTT region size to map, search, and validate
*    poll_interval_ms   Polling interval in milliseconds (default 10ms)
*    log_file           Optional log file path for recording RTT data (deprecated, use log_file_handle)
*    log_file_handle    Optional file handle for recording RTT data
*    run_flag           Optional process run flag; zero requests initialization abort
*    rtt_search_timeout_ms Maximum time to wait for RTTCB discovery, 0 means no timeout
*    reset_memory       Reset backend memory before mapping when supported
*    debug              Enable debug output flag
*/
typedef struct {
    const char     *device_path;
    uint64_t        rtt_address;
    size_t          rtt_region_size;
    unsigned        poll_interval_ms;
    const char     *log_file;
    FILE           *log_file_handle;
    volatile sig_atomic_t *run_flag;
    unsigned        rtt_search_timeout_ms;
    bool            reset_memory;
    bool            debug;
} RTTBridge_Config_t;

/*********************************************************************
*
*       RTTBridge_State_t
*
*  Description
*    Runtime state of the RTT bridge.
*
*  Fields
*    config           Copy of initialization configuration
*    rtt_cb_address   Discovered RTT Control Block local address
*    rtt_region_size  Remaining mapped region size from rtt_cb_address
*    initialized      Bridge initialization state
*    running          Bridge is running flag
*    memory_initialized RTT memory backend initialization state
*    polls_count      Total number of RTT buffer polls
*    errors_count     Total number of errors encountered
*/
typedef struct {
    RTTBridge_Config_t  config;
    uintptr_t           rtt_cb_address;
    size_t              rtt_region_size;
    bool                initialized;
    bool                running;
    bool                memory_initialized;
    unsigned            polls_count;
    unsigned            errors_count;
} RTTBridge_State_t;

/*********************************************************************
*
*       API functions
*
**********************************************************************
*/

int                       RTTBridge_Init                     (RTTBridge_Config_t *config);
void                      RTTBridge_Cleanup                  (void);
const RTTBridge_State_t * RTTBridge_GetState                 (void);
int                       RTTBridge_GetValidatedRTTRegion    (uintptr_t *address, size_t *region_size);
int                       RTTBridge_CheckUpBufferChannel     (unsigned channel);
int                       RTTBridge_CheckDownBufferChannel   (unsigned channel);
int                       RTTBridge_GetBytesInBuffer         (unsigned channel);
int                       RTTBridge_ReadUpBufferNoLock       (unsigned channel, void *buffer, size_t buffer_size);
int                       RTTBridge_WriteDownBufferNoLock    (unsigned channel, const void *buffer, size_t num_bytes);
int                       RTTBridge_EnsureRTTInitialized     (uintptr_t address, size_t region_size, unsigned num_channels);
int                       RTTBridge_WaitForRTTInitialized    (uintptr_t address, unsigned timeout_ms, unsigned retry_interval_ms);
int                       RTTBridge_WaitForRTTUpChannelReady (uintptr_t address, size_t region_size, unsigned channel, unsigned timeout_ms, unsigned retry_interval_ms);
bool                      RTTBridge_IsRunning                (void);
void                      RTTBridge_SetRunning               (bool running);
const char *              RTTBridge_GetLogFile               (void);
FILE *                    RTTBridge_GetLogFileHandle         (void);
void                      RTTBridge_IncrementPolls           (void);
void                      RTTBridge_IncrementErrors          (void);
void                      RTTBridge_Status                   (void);

#if defined(__cplusplus)          // Allow usage of this module from C++ files (disable name mangling)
}                /* Make sure we have C-declarations in C++ programs */
#endif

#endif // end of include guard: TRACEHUB_RTTBRIDGE_H Avoid multiple inclusion

/*************************** End of file ****************************/
