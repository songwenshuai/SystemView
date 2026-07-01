/*********************************************************************
*                             CineLogic                              *
*                      RTT Trace and Debug Bridge                    *
**********************************************************************
----------------------------------------------------------------------
File    : SystemViewState.c
Purpose : SystemView shared state and network queue management
---------------------------END-OF-HEADER------------------------------
*/

#include <stdio.h>

#include "SystemView_internal.h"
#include "Log.h"
#include "RTTBridge.h"

SystemView_State_t _sysview_state;

/*********************************************************************
*
*       _SystemView_GetNetworkQueueSize()
*
*  Function description
*    Return the configured network queue capacity.
*/
size_t _SystemView_GetNetworkQueueSize(void) {
    return SYSTEMVIEW_DEFAULT_NETWORK_QUEUE_SIZE;
}

/*********************************************************************
*
*       _SystemView_IsRunning()
*
*  Function description
*    Read the service running state through the module lock.
*/
bool _SystemView_IsRunning(SystemView_State_t *pState) {
    bool running;

    if (pState == NULL) {
        return false;
    }
    if (!pState->LockInitialized) {
        return pState->Running;
    }

    SYS_MutexLock(&pState->Lock);
    running = pState->Running;
    SYS_MutexUnlock(&pState->Lock);
    return running;
}

/*********************************************************************
*
*       _SystemView_SetRunning()
*
*  Function description
*    Update the service running state through the module lock.
*/
void _SystemView_SetRunning(SystemView_State_t *pState, bool running) {
    if (pState == NULL) {
        return;
    }
    if (!pState->LockInitialized) {
        pState->Running = running;
        return;
    }

    SYS_MutexLock(&pState->Lock);
    pState->Running = running;
    SYS_MutexUnlock(&pState->Lock);
}

/*********************************************************************
*
*       _SystemView_HasFatalError()
*
*  Function description
*    Read the fatal error state through the module lock.
*/
bool _SystemView_HasFatalError(SystemView_State_t *pState) {
    bool fatal_error;

    if (pState == NULL) {
        return false;
    }
    if (!pState->LockInitialized) {
        return pState->FatalError;
    }

    SYS_MutexLock(&pState->Lock);
    fatal_error = pState->FatalError;
    SYS_MutexUnlock(&pState->Lock);
    return fatal_error;
}

/*********************************************************************
*
*       _SystemView_ReportFatalServiceError()
*
*  Function description
*    Mark an unrecoverable SystemView service failure and stop the service.
*/
void _SystemView_ReportFatalServiceError(SystemView_State_t *pState,
                                         const char *operation) {
    bool should_report;

    if (pState == NULL) {
        return;
    }

    if (operation == NULL) {
        operation = "access";
    }

    should_report = false;
    if (pState->LockInitialized) {
        SYS_MutexLock(&pState->Lock);
    }
    if (!pState->FatalError) {
        pState->FatalError = true;
        pState->Running = false;
        should_report = true;
    } else {
        pState->Running = false;
    }
    if (pState->LockInitialized) {
        SYS_MutexUnlock(&pState->Lock);
    }

    if (should_report) {
        Log_Error("SystemView: %s failed on channel %u\n",
                  operation,
                  pState->Config.channel);
    }
}

/*********************************************************************
*
*       _SystemView_ReportRecoverableRTTError()
*
*  Function description
*    Mark RTT access as temporarily unavailable without entering fatal state.
*/
void _SystemView_ReportRecoverableRTTError(SystemView_State_t *pState,
                                           const char *operation) {
    bool should_report;

    if (pState == NULL) {
        return;
    }
    if (operation == NULL) {
        operation = "access";
    }

    should_report = false;
    SYS_MutexLock(&pState->Lock);
    if (!pState->RTTRecovering) {
        pState->RTTRecovering = true;
        _SystemView_RequestNetworkStreamResetLocked(pState);
        should_report = true;
    }
    SYS_MutexUnlock(&pState->Lock);

    if (should_report) {
        Log_Warn("SystemView: RTT %s failed on channel %u; waiting for target recovery\n",
                 operation,
                 pState->Config.channel);
    }
}

/*********************************************************************
*
*       _SystemView_ClearNetworkQueueLocked()
*
*  Function description
*    Clear queued live network trace bytes. Caller must hold pState->Lock.
*/
void _SystemView_ClearNetworkQueueLocked(SystemView_State_t *pState) {
    if (pState == NULL) {
        return;
    }

    ByteQueue_Clear(&pState->NetworkQueue);
}

/*********************************************************************
*
*       _SystemView_RequestNetworkStreamResetLocked()
*
*  Function description
*    Drop live network stream state after target RTT continuity is lost.
*    Caller must hold pState->Lock.
*/
void _SystemView_RequestNetworkStreamResetLocked(SystemView_State_t *pState) {
    if (pState == NULL || !pState->Config.network_enabled) {
        return;
    }

    _SystemView_ClearNetworkQueueLocked(pState);
    pState->ClientDisconnectRequested = true;
}

/*********************************************************************
*
*       _SystemView_ReportRTTRecovered()
*
*  Function description
*    Report transition from RTT recovery wait back to normal operation.
*/
void _SystemView_ReportRTTRecovered(SystemView_State_t *pState) {
    bool should_report;

    if (pState == NULL) {
        return;
    }

    should_report = false;
    SYS_MutexLock(&pState->Lock);
    if (pState->RTTRecovering) {
        pState->RTTRecovering = false;
        should_report = true;
    }
    SYS_MutexUnlock(&pState->Lock);

    if (should_report) {
        Log_Warn("SystemView: RTT access recovered on channel %u\n",
                 pState->Config.channel);
    }
}

/*********************************************************************
*
*       _SystemView_CheckChannels()
*
*  Function description
*    Verify all RTT channels required by the configured SystemView modes.
*/
int _SystemView_CheckChannels(SystemView_State_t *pState) {
    if (pState == NULL) {
        return -1;
    }

    if (RTTBridge_CheckUpBufferChannel(pState->Config.channel) != 0) {
        Log_Error("SystemView: RTT up-buffer channel %u is not configured\n",
                  pState->Config.channel);
        return -1;
    }
    if (pState->Config.network_enabled &&
        (RTTBridge_CheckDownBufferChannel(pState->Config.channel) != 0)) {
        Log_Error("SystemView: RTT down-buffer channel %u is not configured\n",
                  pState->Config.channel);
        return -1;
    }

    return 0;
}

/*********************************************************************
*
*       _SystemView_GetClient()
*
*  Function description
*    Read the active client handle through the module lock.
*/
SYS_SOCKET_HANDLE _SystemView_GetClient(SystemView_State_t *pState) {
    SYS_SOCKET_HANDLE hClient;

    if (pState == NULL) {
        return SYS_SOCKET_INVALID_HANDLE;
    }
    if (!pState->LockInitialized) {
        return pState->hClient;
    }

    SYS_MutexLock(&pState->Lock);
    hClient = pState->hClient;
    SYS_MutexUnlock(&pState->Lock);
    return hClient;
}

/*********************************************************************
*
*       _SystemView_ResetConnectionStateLocked()
*
*  Function description
*    Reset per-client transfer state. Caller must hold pState->Lock.
*/
void _SystemView_ResetConnectionStateLocked(SystemView_State_t *pState) {
    pState->SendNumBytes      = 0;
    pState->WriteNumBytes     = 0;
    pState->AppPacketExpected = 0u;
    pState->AppPacketReceived = 0u;
    pState->pSendBuf          = NULL;
    pState->pWriteBuf         = NULL;
}

/*********************************************************************
*
*       _SystemView_SetConnectedClient()
*
*  Function description
*    Publish a new client after the SystemView handshake completes.
*/
unsigned _SystemView_SetConnectedClient(SystemView_State_t *pState, SYS_SOCKET_HANDLE hClient) {
    unsigned ConnectionsCount;

    if (pState == NULL) {
        return 0u;
    }

    SYS_MutexLock(&pState->Lock);
    pState->hClient = hClient;
    pState->HandshakeDone = true;
    pState->ClientDisconnectRequested = false;
    _SystemView_ResetConnectionStateLocked(pState);
    pState->ConnectionsCount++;
    ConnectionsCount = pState->ConnectionsCount;
    SYS_MutexUnlock(&pState->Lock);
    return ConnectionsCount;
}

/*********************************************************************
*
*       _SystemView_AddBytesSent()
*
*  Function description
*    Add sent byte count to SystemView service statistics.
*
*  Parameters
*    pState    SystemView service state.
*    NumBytes  Number of bytes sent.
*/
void _SystemView_AddBytesSent(SystemView_State_t *pState, unsigned NumBytes) {
    if ((pState == NULL) || (NumBytes == 0u)) {
        return;
    }

    SYS_MutexLock(&pState->Lock);
    pState->BytesSent += NumBytes;
    SYS_MutexUnlock(&pState->Lock);
}

/*********************************************************************
*
*       _SystemView_AddBytesReceived()
*
*  Function description
*    Add received byte count to SystemView service statistics.
*
*  Parameters
*    pState    SystemView service state.
*    NumBytes  Number of bytes received.
*/
void _SystemView_AddBytesReceived(SystemView_State_t *pState, unsigned NumBytes) {
    if ((pState == NULL) || (NumBytes == 0u)) {
        return;
    }

    SYS_MutexLock(&pState->Lock);
    pState->BytesReceived += NumBytes;
    SYS_MutexUnlock(&pState->Lock);
}

/*********************************************************************
*
*       _SystemView_QueueTargetData()
*
*  Function description
*    Queue recorded target trace bytes for network clients.
*    Network queue overflow invalidates the live network stream only;
*    local SVDat recording remains authoritative.
*
*  Return value
*     0  Queued or network mode disabled
*    -1  Single target trace batch exceeds queue capacity
*/
int _SystemView_QueueTargetData(SystemView_State_t *pState, const char *data, unsigned num_bytes) {
    ByteQueue_WriteResult_t queue_result;

    if ((pState == NULL) || (data == NULL) || (num_bytes == 0u)) {
        return 0;
    }

    SYS_MutexLock(&pState->Lock);

    if (!pState->Config.network_enabled) {
        SYS_MutexUnlock(&pState->Lock);
        return 0;
    }
    if (!ByteQueue_IsValid(&pState->NetworkQueue)) {
        SYS_MutexUnlock(&pState->Lock);
        _SystemView_ReportFatalServiceError(pState, "network queue");
        return -1;
    }

    queue_result = ByteQueue_Write(&pState->NetworkQueue, data, (size_t)num_bytes);
    if (queue_result == BYTE_QUEUE_WRITE_OVERFLOW_WRITTEN) {
        pState->ClientDisconnectRequested = true;
    }

    if (queue_result == BYTE_QUEUE_WRITE_TOO_LARGE) {
        SYS_MutexUnlock(&pState->Lock);
        Log_Error("SystemView: target trace batch exceeds network queue capacity "
                  "(requested=%u, capacity=%zu)\n",
                  num_bytes,
                  ByteQueue_GetCapacity(&pState->NetworkQueue));
        _SystemView_ReportFatalServiceError(pState, "network queue");
        return -1;
    }

    SYS_MutexUnlock(&pState->Lock);

    if (queue_result == BYTE_QUEUE_WRITE_OVERFLOW_WRITTEN) {
        if (pState->Config.record_enabled) {
            Log_Warn("SystemView: network trace queue capacity exceeded; "
                     "dropping network backlog and closing current client; "
                     "local SVDat recording continues\n");
        } else {
            Log_Warn("SystemView: network trace queue capacity exceeded; "
                     "dropping network backlog and closing current client\n");
        }
    }
    return 0;
}

/*********************************************************************
*
*       _SystemView_DequeueTargetData()
*
*  Function description
*    Copy queued network trace bytes to the caller buffer and remove them
*    under the same lock. The caller owns the copied batch after this call.
*
*  Return value
*    Number of bytes copied.
*/
unsigned _SystemView_DequeueTargetData(SystemView_State_t *pState, char *buffer, unsigned buffer_size) {
    size_t   num_bytes;
    unsigned result;

    if ((pState == NULL) || (buffer == NULL) || (buffer_size == 0u)) {
        return 0u;
    }
    if (!ByteQueue_IsValid(&pState->NetworkQueue)) {
        return 0u;
    }

    SYS_MutexLock(&pState->Lock);
    num_bytes = ByteQueue_Read(&pState->NetworkQueue, buffer, (size_t)buffer_size);
    SYS_MutexUnlock(&pState->Lock);

    result = (unsigned)num_bytes;
    return result;
}

/*********************************************************************
*
*       _SystemView_CloseClient()
*
*  Function description
*    Close the active client socket and reset per-connection transfer state.
*
*  Parameters
*    pState - SystemView service state
*/
void _SystemView_CloseClient(SystemView_State_t *pState) {
    SYS_SOCKET_HANDLE hClient;

    if (pState == NULL) {
        return;
    }

    SYS_MutexLock(&pState->Lock);
    hClient = pState->hClient;
    pState->hClient = SYS_SOCKET_INVALID_HANDLE;
    pState->HandshakeDone = false;
    pState->ClientDisconnectRequested = false;
    _SystemView_ResetConnectionStateLocked(pState);
    _SystemView_ClearNetworkQueueLocked(pState);
    SYS_MutexUnlock(&pState->Lock);

    if (hClient != SYS_SOCKET_INVALID_HANDLE) {
        SYS_SOCKET_Shutdown(hClient, SYS_SOCKET_SHUT_RDWR);
        SYS_SOCKET_Close(hClient);
    }
}

/*********************************************************************
*
*       _SystemView_TakeClientDisconnectRequest()
*
*  Function description
*    Consume a pending request to close the current client.
*/
bool _SystemView_TakeClientDisconnectRequest(SystemView_State_t *pState) {
    bool requested;

    if (pState == NULL) {
        return false;
    }

    SYS_MutexLock(&pState->Lock);
    requested = pState->ClientDisconnectRequested;
    pState->ClientDisconnectRequested = false;
    SYS_MutexUnlock(&pState->Lock);
    return requested;
}

/*********************************************************************
*
*       _SystemView_CloseClientForNetworkDisconnect()
*
*  Function description
*    Close the current network client after the peer closes the connection.
*/
void _SystemView_CloseClientForNetworkDisconnect(SystemView_State_t *pState,
                                                 const char *operation) {
    if (operation == NULL) {
        operation = "client disconnected";
    }

    Log_Info("SystemView: network %s; closing current client\n", operation);
    _SystemView_CloseClient(pState);
}

/*********************************************************************
*
*       _SystemView_CloseClientForNetworkError()
*
*  Function description
*    Close the current network client after a recoverable socket error.
*/
void _SystemView_CloseClientForNetworkError(SystemView_State_t *pState,
                                            const char *operation) {
    if (operation == NULL) {
        operation = "connection";
    }

    Log_Warn("SystemView: network %s; closing current client\n", operation);
    _SystemView_CloseClient(pState);
}


/*************************** End of file ****************************/
