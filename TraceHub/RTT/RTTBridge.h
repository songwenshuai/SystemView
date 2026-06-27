/*********************************************************************
*                                                                    *
*                Copyright (C) 2023 xrTest Inc.                      *
*                      All rights reserved                           *
*                                                                    *
**********************************************************************
----------------------------------------------------------------------
File    : RTTBridge.h
Purpose : RTT Bridge core state and configuration management
Author  : songwenshuai <songwenshuai@gmail.com>
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

/*********************************************************************
*
*       RTTBridge_Init()
*
*  Function description
*    Initialize the RTT bridge with the specified configuration.
*    Sets up memory mapping and locates RTT Control Block.
*
*  Parameters
*    config  Pointer to configuration structure
*
*  Return value
*    0      Success
*   -1      Failed to setup memory mapping
*   -2      Failed to find RTT Control Block
*/
int RTTBridge_Init(RTTBridge_Config_t *config);

/*********************************************************************
*
*       RTTBridge_Cleanup()
*
*  Function description
*    Cleanup RTT bridge resources and unmap RTT memory.
*/
void RTTBridge_Cleanup(void);

/*********************************************************************
*
*       RTTBridge_GetState()
*
*  Function description
*    Get pointer to the bridge state structure.
*
*  Return value
*    Read-only pointer to RTTBridge_State_t structure
*/
const RTTBridge_State_t* RTTBridge_GetState(void);

/*********************************************************************
*
*       RTTBridge_GetValidatedRTTRegion()
*
*  Function description
*    Get the current local mapped RTT control block address after
*    validating the current RTT region. If the stored address is invalid,
*    the configured mapped backend range is searched again.
*
*  Parameters
*    address      Pointer to store local mapped RTT control block address
*    region_size  Optional pointer to store remaining mapped region size
*
*  Return value
*    0   Success
*   -1   RTT region is not initialized or invalid
*/
int RTTBridge_GetValidatedRTTRegion(uintptr_t *address, size_t *region_size);

/*********************************************************************
*
*       RTTBridge_CheckUpBufferChannel()
*
*  Function description
*    Check whether the current mapped RTT region contains a configured
*    up-buffer channel.
*
*  Parameters
*    channel  RTT up-buffer channel index
*
*  Return value
*    0   Up-buffer channel is configured
*   -1   RTT region or up-buffer channel is invalid
*/
int RTTBridge_CheckUpBufferChannel(unsigned channel);

/*********************************************************************
*
*       RTTBridge_CheckDownBufferChannel()
*
*  Function description
*    Check whether the current mapped RTT region contains a configured
*    down-buffer channel.
*
*  Parameters
*    channel  RTT down-buffer channel index
*
*  Return value
*    0   Down-buffer channel is configured
*   -1   RTT region or down-buffer channel is invalid
*/
int RTTBridge_CheckDownBufferChannel(unsigned channel);

/*********************************************************************
*
*       RTTBridge_GetBytesInBuffer()
*
*  Function description
*    Get the number of pending bytes in an RTT up-buffer after validating
*    the mapped RTT region.
*
*  Parameters
*    channel  RTT up-buffer channel index
*
*  Return value
*    >= 0  Number of pending bytes
*    -1    RTT region is not initialized or invalid
*/
int RTTBridge_GetBytesInBuffer(unsigned channel);

/*********************************************************************
*
*       RTTBridge_ReadUpBufferNoLock()
*
*  Function description
*    Read from an RTT up-buffer after validating the mapped RTT region.
*
*  Parameters
*    channel      RTT up-buffer channel index
*    buffer       Destination buffer
*    buffer_size  Destination buffer size in bytes
*
*  Return value
*    >= 0  Number of bytes read
*    -1    RTT region is not initialized or invalid
*/
int RTTBridge_ReadUpBufferNoLock(unsigned channel, void *buffer, size_t buffer_size);

/*********************************************************************
*
*       RTTBridge_WriteDownBufferNoLock()
*
*  Function description
*    Write to an RTT down-buffer after validating the mapped RTT region.
*
*  Parameters
*    channel    RTT down-buffer channel index
*    buffer     Source buffer
*    num_bytes  Number of bytes to write
*
*  Return value
*    >= 0  Number of bytes written
*    -1    RTT region is not initialized or invalid
*/
int RTTBridge_WriteDownBufferNoLock(unsigned channel, const void *buffer, size_t num_bytes);

/*********************************************************************
*
*       RTTBridge_EnsureRTTInitialized()
*
*  Function description
*    Initialize or validate a local mapped RTT control block through
*    SEGGER RTT and configure the requested up/down channels.
*
*  Parameters
*    address       Local mapped address of RTT control block
*    region_size   Remaining mapped region size from address
*    num_channels  Number of RTT channels to configure
*
*  Return value
*    0   Success
*   -1   Failed
*/
int RTTBridge_EnsureRTTInitialized(uintptr_t address, size_t region_size, unsigned num_channels);

/*********************************************************************
*
*       RTTBridge_WaitForRTTInitialized()
*
*  Function description
*    Wait until a local mapped RTT control block has been initialized.
*
*  Parameters
*    address            Local mapped address of RTT control block
*    timeout_ms         Timeout in milliseconds, 0 for no timeout
*    retry_interval_ms  Retry interval in milliseconds
*
*  Return value
*    0   Success
*   -1   Failed
*/
int RTTBridge_WaitForRTTInitialized(uintptr_t address, unsigned timeout_ms, unsigned retry_interval_ms);

/*********************************************************************
*
*       RTTBridge_WaitForRTTUpChannelReady()
*
*  Function description
*    Wait until a local mapped RTT control block is valid and the
*    specified up-buffer channel has been configured.
*
*  Parameters
*    address            Local mapped address of RTT control block
*    region_size        Remaining mapped region size from address
*    channel            RTT up-buffer channel index
*    timeout_ms         Timeout in milliseconds, 0 for no timeout
*    retry_interval_ms  Retry interval in milliseconds
*
*  Return value
*    0   Success
*   -1   Failed
*/
int RTTBridge_WaitForRTTUpChannelReady(uintptr_t address, size_t region_size,
                                       unsigned channel, unsigned timeout_ms,
                                       unsigned retry_interval_ms);

/*********************************************************************
*
*       RTTBridge_IsRunning()
*
*  Function description
*    Check if the bridge is currently running.
*
*  Return value
*    true   Bridge is running
*    false  Bridge is not running
*/
bool RTTBridge_IsRunning(void);

/*********************************************************************
*
*       RTTBridge_SetRunning()
*
*  Function description
*    Set the bridge running state.
*
*  Parameters
*    running  New running state
*/
void RTTBridge_SetRunning(bool running);

/*********************************************************************
*
*       RTTBridge_GetLogFile()
*
*  Function description
*    Get the configured log file path (deprecated).
*
*  Return value
*    Log file path, or NULL if not configured
*/
const char* RTTBridge_GetLogFile(void);

/*********************************************************************
*
*       RTTBridge_GetLogFileHandle()
*
*  Function description
*    Get the configured log file handle.
*
*  Return value
*    Log file handle, or NULL if not configured
*/
FILE* RTTBridge_GetLogFileHandle(void);

/*********************************************************************
*
*       RTTBridge_IncrementPolls()
*
*  Function description
*    Thread-safe increment of polls counter.
*/
void RTTBridge_IncrementPolls(void);

/*********************************************************************
*
*       RTTBridge_IncrementErrors()
*
*  Function description
*    Thread-safe increment of errors counter.
*/
void RTTBridge_IncrementErrors(void);

/*********************************************************************
*
*       RTTBridge_Status()
*
*  Function description
*    Print current bridge status to stdout.
*/
void RTTBridge_Status(void);

#if defined(__cplusplus)          // Allow usage of this module from C++ files (disable name mangling)
}                /* Make sure we have C-declarations in C++ programs */
#endif

#endif // end of include guard: TRACEHUB_RTTBRIDGE_H Avoid multiple inclusion

/*************************** End of file ****************************/
