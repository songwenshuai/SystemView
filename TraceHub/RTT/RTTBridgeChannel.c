/*********************************************************************
*                             CineLogic                              *
*                      RTT Trace and Debug Bridge                    *
**********************************************************************
----------------------------------------------------------------------
File    : RTTBridgeChannel.c
Purpose : RTT bridge channel validation and data transfer
---------------------------END-OF-HEADER------------------------------
*/

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include <limits.h>

#include "RTTBridge_internal.h"
#include "SEGGER_RTT.h"

/*********************************************************************
*
*       Public functions
*
**********************************************************************
*/

/*********************************************************************
*
*       RTTBridge_EnsureRTTInitialized()
*
*  Function description
*    Initialize or validate a local mapped RTT control block through
*    SEGGER RTT and configure the requested up/down channels.
*
*  Parameters
*    address       Local mapped address of RTT control block.
*    region_size   Remaining mapped region size from address.
*    num_channels  Number of RTT channels to configure.
*
*  Return value
*    0   Success.
*   -1   Failed.
*/
int RTTBridge_EnsureRTTInitialized(uintptr_t address, size_t region_size, unsigned num_channels) {
    return SEGGER_RTT_EnsureInitEx(address, region_size, num_channels);
}

/*********************************************************************
*
*       RTTBridge_WaitForRTTInitialized()
*
*  Function description
*    Wait until a local mapped RTT control block has been initialized.
*
*  Parameters
*    address            Local mapped address of RTT control block.
*    timeout_ms         Timeout in milliseconds, 0 for no timeout.
*    retry_interval_ms  Retry interval in milliseconds.
*
*  Return value
*    0   Success.
*   -1   Failed.
*/
int RTTBridge_WaitForRTTInitialized(uintptr_t address, unsigned timeout_ms, unsigned retry_interval_ms) {
    unsigned elapsed_ms;

    if ((address == 0u) || (retry_interval_ms == 0u)) {
        return -1;
    }

    elapsed_ms = 0u;
    while (SEGGER_RTT_CheckInit(address) != 0) {
        if ((timeout_ms > 0u) && (elapsed_ms >= timeout_ms)) {
            return -1;
        }
        SYS_Sleep(retry_interval_ms);
        if ((timeout_ms > 0u) && (retry_interval_ms > (UINT_MAX - elapsed_ms))) {
            elapsed_ms = timeout_ms;
        } else {
            elapsed_ms += retry_interval_ms;
        }
    }

    return 0;
}

/*********************************************************************
*
*       RTTBridge_WaitForRTTUpChannelReady()
*
*  Function description
*    Wait until a local mapped RTT control block is valid and the
*    specified up-buffer channel has been configured.
*
*  Parameters
*    address            Local mapped address of RTT control block.
*    region_size        Remaining mapped region size from address.
*    channel            RTT up-buffer channel index.
*    timeout_ms         Timeout in milliseconds, 0 for no timeout.
*    retry_interval_ms  Retry interval in milliseconds.
*
*  Return value
*    0   Success.
*   -1   Failed.
*/
int RTTBridge_WaitForRTTUpChannelReady(uintptr_t address, size_t region_size,
                                       unsigned channel, unsigned timeout_ms,
                                       unsigned retry_interval_ms) {
    unsigned elapsed_ms;

    if ((address == 0u) || (region_size == 0u) || (retry_interval_ms == 0u)) {
        return -1;
    }

    elapsed_ms = 0u;
    while ((SEGGER_RTT_CheckRegion(address, region_size) != 0) ||
           (SEGGER_RTT_CheckUpBuffer(address, region_size, channel) != 0)) {
        if ((timeout_ms > 0u) && (elapsed_ms >= timeout_ms)) {
            return -1;
        }
        SYS_Sleep(retry_interval_ms);
        if ((timeout_ms > 0u) && (retry_interval_ms > (UINT_MAX - elapsed_ms))) {
            elapsed_ms = timeout_ms;
        } else {
            elapsed_ms += retry_interval_ms;
        }
    }

    return 0;
}

/*********************************************************************
*
*       RTTBridge_CheckUpBufferChannel()
*
*  Function description
*    Check whether the current mapped RTT region contains a configured
*    up-buffer channel.
*
*  Parameters
*    channel  RTT up-buffer channel index.
*
*  Return value
*    0   Up-buffer channel is configured.
*   -1   RTT region or up-buffer channel is invalid.
*/
int RTTBridge_CheckUpBufferChannel(unsigned channel) {
    uintptr_t address;
    size_t    region_size;

    if (RTTBridge_GetValidatedRTTRegion(&address, &region_size) != 0) {
        return -1;
    }
    if (SEGGER_RTT_CheckUpBuffer(address, region_size, channel) != 0) {
        return -1;
    }

    return 0;
}

/*********************************************************************
*
*       RTTBridge_CheckDownBufferChannel()
*
*  Function description
*    Check whether the current mapped RTT region contains a configured
*    down-buffer channel.
*
*  Parameters
*    channel  RTT down-buffer channel index.
*
*  Return value
*    0   Down-buffer channel is configured.
*   -1   RTT region or down-buffer channel is invalid.
*/
int RTTBridge_CheckDownBufferChannel(unsigned channel) {
    uintptr_t address;
    size_t    region_size;

    if (RTTBridge_GetValidatedRTTRegion(&address, &region_size) != 0) {
        return -1;
    }
    if (SEGGER_RTT_CheckDownBuffer(address, region_size, channel) != 0) {
        return -1;
    }

    return 0;
}

/*********************************************************************
*
*       RTTBridge_GetBytesInBuffer()
*
*  Function description
*    Get the number of pending bytes in an RTT up-buffer after validating
*    the mapped RTT region.
*
*  Parameters
*    channel  RTT up-buffer channel index.
*
*  Return value
*    >= 0  Number of pending bytes.
*    -1    RTT region is not initialized or invalid.
*/
int RTTBridge_GetBytesInBuffer(unsigned channel) {
    uintptr_t address;
    size_t    region_size;
    unsigned  bytes_in_buffer;

    if (RTTBridge_GetValidatedRTTRegion(&address, &region_size) != 0) {
        return -1;
    }
    if (SEGGER_RTT_CheckUpBuffer(address, region_size, channel) != 0) {
        return -1;
    }
    bytes_in_buffer = SEGGER_RTT_GetBytesInBuffer(address, channel);
    if (bytes_in_buffer > (unsigned)INT_MAX) {
        return -1;
    }
    return (int)bytes_in_buffer;
}

/*********************************************************************
*
*       RTTBridge_ReadUpBufferNoLock()
*
*  Function description
*    Read from an RTT up-buffer after validating the mapped RTT region.
*
*  Parameters
*    channel      RTT up-buffer channel index.
*    buffer       Destination buffer.
*    buffer_size  Destination buffer size in bytes.
*
*  Return value
*    >= 0  Number of bytes read.
*    -1    RTT region is not initialized or invalid.
*/
int RTTBridge_ReadUpBufferNoLock(unsigned channel, void *buffer, size_t buffer_size) {
    uintptr_t address;
    size_t    region_size;
    unsigned  bytes_read;

    if ((buffer == NULL) || (buffer_size > (size_t)INT_MAX)) {
        return -1;
    }
    if (RTTBridge_GetValidatedRTTRegion(&address, &region_size) != 0) {
        return -1;
    }
    if (SEGGER_RTT_CheckUpBuffer(address, region_size, channel) != 0) {
        return -1;
    }
    bytes_read = SEGGER_RTT_ReadUpBufferNoLock(address, channel, buffer, (unsigned)buffer_size);
    if (bytes_read > (unsigned)INT_MAX) {
        return -1;
    }
    return (int)bytes_read;
}

/*********************************************************************
*
*       RTTBridge_WriteDownBufferNoLock()
*
*  Function description
*    Write to an RTT down-buffer after validating the mapped RTT region.
*
*  Parameters
*    channel    RTT down-buffer channel index.
*    buffer     Source buffer.
*    num_bytes  Number of bytes to write.
*
*  Return value
*    >= 0  Number of bytes written.
*    -1    RTT region is not initialized or invalid.
*/
int RTTBridge_WriteDownBufferNoLock(unsigned channel, const void *buffer, size_t num_bytes) {
    uintptr_t address;
    size_t    region_size;
    unsigned  bytes_written;

    if ((buffer == NULL) || (num_bytes > (size_t)INT_MAX)) {
        return -1;
    }
    if (RTTBridge_GetValidatedRTTRegion(&address, &region_size) != 0) {
        return -1;
    }
    if (SEGGER_RTT_CheckDownBuffer(address, region_size, channel) != 0) {
        return -1;
    }
    bytes_written = SEGGER_RTT_WriteDownBufferNoLock(address, channel, buffer, (unsigned)num_bytes);
    if (bytes_written > (unsigned)INT_MAX) {
        return -1;
    }
    return (int)bytes_written;
}

/*************************** End of file ****************************/
