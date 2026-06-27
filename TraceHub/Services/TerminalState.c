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
File    : TerminalState.c
Purpose : Terminal shared state and queue management
---------------------------END-OF-HEADER------------------------------
*/

#include <stdio.h>

#include "Terminal_internal.h"
#include "CoreLogRecorder.h"
#include "Log.h"
#include "RTTBridge.h"

Terminal_State_t _terminal_state;

/*********************************************************************
*
*       _Terminal_GetConfiguredQueueSize()
*
*  Function description
*    Return the configured network queue capacity.
*/
size_t _Terminal_GetNetworkQueueSize(void) {
    return TERMINAL_DEFAULT_NETWORK_QUEUE_SIZE;
}

/*********************************************************************
*
*       _Terminal_IsRunning()
*
*  Function description
*    Read the service running state through the module lock.
*/
bool _Terminal_IsRunning(Terminal_State_t *pState) {
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
*       _Terminal_SetRunning()
*
*  Function description
*    Update the service running state through the module lock.
*/
void _Terminal_SetRunning(Terminal_State_t *pState, bool running) {
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
*       _Terminal_HasFatalError()
*
*  Function description
*    Read the fatal error state through the module lock.
*/
bool _Terminal_HasFatalError(Terminal_State_t *pState) {
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
*       _Terminal_ReportRTTError()
*
*  Function description
*    Record an unrecoverable RTT access error and stop service threads.
*/
void _Terminal_ReportRTTError(Terminal_State_t *pState, const char *operation) {
    bool should_report;

    if (pState == NULL) {
        return;
    }
    if (operation == NULL) {
        operation = "operation";
    }

    if (pState->LockInitialized) {
        SYS_MutexLock(&pState->Lock);
        should_report = !pState->FatalError;
        pState->FatalError = true;
        pState->Running = false;
        SYS_MutexUnlock(&pState->Lock);
    } else {
        should_report = !pState->FatalError;
        pState->FatalError = true;
        pState->Running = false;
    }

    if (should_report) {
        Log_Error("Terminal: fatal RTT %s failure on channel %u\n",
                  operation, pState->Config.channel);
    }
}

/*********************************************************************
*
*       _Terminal_GetClient()
*
*  Function description
*    Read the active client handle through the module lock.
*/
SYS_SOCKET_HANDLE _Terminal_GetClient(Terminal_State_t *pState) {
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
*       _Terminal_ResetConnectionStateLocked()
*
*  Function description
*    Reset per-client transfer state. Caller must hold pState->Lock.
*/
void _Terminal_ResetConnectionStateLocked(Terminal_State_t *pState) {
    pState->SendNumBytes  = 0;
    pState->WriteNumBytes = 0;
    pState->pSendBuf      = NULL;
    pState->pWriteBuf     = NULL;
    TelnetCodec_Reset(&pState->TelnetState);
}

/*********************************************************************
*
*       _Terminal_ClearNetworkQueueLocked()
*
*  Function description
*    Clear queued live network bytes. Caller must hold pState->Lock.
*/
void _Terminal_ClearNetworkQueueLocked(Terminal_State_t *pState) {
    if (pState == NULL) {
        return;
    }

    ByteQueue_Clear(&pState->NetworkQueue);
}

/*********************************************************************
*
*       _Terminal_RequestNetworkStreamResetLocked()
*
*  Function description
*    Drop live TCP stream state after target RTT continuity is lost.
*    Caller must hold pState->Lock.
*/
void _Terminal_RequestNetworkStreamResetLocked(Terminal_State_t *pState) {
    if (pState == NULL || pState->Config.console_mode) {
        return;
    }

    _Terminal_ClearNetworkQueueLocked(pState);
    pState->ClientDisconnectRequested = true;
}

/*********************************************************************
*
*       _Terminal_ReportRecoverableRTTError()
*
*  Function description
*    Mark RTT access as temporarily unavailable without entering fatal state.
*/
void _Terminal_ReportRecoverableRTTError(Terminal_State_t *pState, const char *operation) {
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
        _Terminal_RequestNetworkStreamResetLocked(pState);
        should_report = true;
    }
    SYS_MutexUnlock(&pState->Lock);

    if (should_report) {
        Log_Warn("Terminal: RTT %s failed on channel %u; waiting for target recovery\n",
                 operation,
                 pState->Config.channel);
    }
}

/*********************************************************************
*
*       _Terminal_ReportRTTRecovered()
*
*  Function description
*    Report transition from RTT recovery wait back to normal operation.
*/
void _Terminal_ReportRTTRecovered(Terminal_State_t *pState) {
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
        Log_Warn("Terminal: RTT access recovered on channel %u\n",
                 pState->Config.channel);
    }
}

/*********************************************************************
*
*       _Terminal_SetConnectedClient()
*
*  Function description
*    Publish a new client and reset per-client transfer state.
*/
unsigned _Terminal_SetConnectedClient(Terminal_State_t *pState, SYS_SOCKET_HANDLE hClient) {
    unsigned ConnectionsCount;

    if (pState == NULL) {
        return 0u;
    }

    SYS_MutexLock(&pState->Lock);
    pState->hClient = hClient;
    pState->ClientDisconnectRequested = false;
    _Terminal_ResetConnectionStateLocked(pState);
    pState->ConnectionsCount++;
    ConnectionsCount = pState->ConnectionsCount;
    SYS_MutexUnlock(&pState->Lock);
    return ConnectionsCount;
}

/*********************************************************************
*
*       _Terminal_AddBytesSent()
*
*  Function description
*    Add sent byte count to terminal service statistics.
*
*  Parameters
*    pState    Terminal service state.
*    NumBytes  Number of bytes sent.
*/
void _Terminal_AddBytesSent(Terminal_State_t *pState, unsigned NumBytes) {
    if ((pState == NULL) || (NumBytes == 0u)) {
        return;
    }

    SYS_MutexLock(&pState->Lock);
    pState->BytesSent += NumBytes;
    SYS_MutexUnlock(&pState->Lock);
}

/*********************************************************************
*
*       _Terminal_AddBytesReceived()
*
*  Function description
*    Add received byte count to terminal service statistics.
*
*  Parameters
*    pState    Terminal service state.
*    NumBytes  Number of bytes received.
*/
void _Terminal_AddBytesReceived(Terminal_State_t *pState, unsigned NumBytes) {
    if ((pState == NULL) || (NumBytes == 0u)) {
        return;
    }

    SYS_MutexLock(&pState->Lock);
    pState->BytesReceived += NumBytes;
    SYS_MutexUnlock(&pState->Lock);
}

/*********************************************************************
*
*       _Terminal_QueueTargetData()
*
*  Function description
*    Queue RTT target output for TCP clients.
*    The queue is a live network backlog. Overflow drops the backlog and
*    requests the current client to be closed.
*
*  Return value
*     0  Queued
*    -1  Single target output batch exceeds queue capacity
*/
int _Terminal_QueueTargetData(Terminal_State_t *pState, const char *data, unsigned num_bytes) {
    ByteQueue_WriteResult_t queue_result;

    if ((pState == NULL) || (data == NULL) || (num_bytes == 0u)) {
        return 0;
    }
    if (!ByteQueue_IsValid(&pState->NetworkQueue)) {
        _Terminal_ReportRTTError(pState, "network queue");
        return -1;
    }

    SYS_MutexLock(&pState->Lock);

    queue_result = ByteQueue_Write(&pState->NetworkQueue, data, (size_t)num_bytes);
    if (queue_result == BYTE_QUEUE_WRITE_OVERFLOW_WRITTEN) {
        pState->ClientDisconnectRequested = true;
    }

    if (queue_result == BYTE_QUEUE_WRITE_TOO_LARGE) {
        SYS_MutexUnlock(&pState->Lock);
        Log_Error("Terminal: target output batch exceeds network queue capacity "
                  "(requested=%u, capacity=%zu)\n",
                  num_bytes,
                  ByteQueue_GetCapacity(&pState->NetworkQueue));
        _Terminal_ReportRTTError(pState, "network queue");
        return -1;
    }

    SYS_MutexUnlock(&pState->Lock);

    if (queue_result == BYTE_QUEUE_WRITE_OVERFLOW_WRITTEN) {
        Log_Warn("Terminal: network output queue capacity exceeded; "
                 "dropping network backlog and closing current client\n");
    }
    return 0;
}

/*********************************************************************
*
*       _Terminal_DequeueTargetData()
*
*  Function description
*    Copy queued RTT target output to the caller buffer and remove it
*    under the same lock.
*
*  Return value
*    Number of bytes copied.
*/
unsigned _Terminal_DequeueTargetData(Terminal_State_t *pState, char *buffer, unsigned buffer_size) {
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
*       _Terminal_RegisterCoreConsumer()
*
*  Function description
*    Register Terminal as the single consumer when using a core channel.
*/
int _Terminal_RegisterCoreConsumer(Terminal_State_t *pState) {
    int result;

    if (pState == NULL) {
        return -1;
    }
    if (!CoreLogRecorder_IsCoreChannel(pState->Config.channel)) {
        pState->CoreConsumerRegistered = false;
        return 0;
    }

    result = CoreLogRecorder_RegisterConsumer(pState->Config.channel);
    if (result != 0) {
        Log_Error("Terminal: core channel %u already has a consumer\n",
                  pState->Config.channel);
        return -1;
    }

    pState->CoreConsumerRegistered = true;
    return 0;
}

/*********************************************************************
*
*       _Terminal_UnregisterCoreConsumer()
*
*  Function description
*    Release Terminal core recorder consumer registration.
*/
void _Terminal_UnregisterCoreConsumer(Terminal_State_t *pState) {
    if (pState == NULL || !pState->CoreConsumerRegistered) {
        return;
    }

    CoreLogRecorder_UnregisterConsumer(pState->Config.channel);
    pState->CoreConsumerRegistered = false;
}

/*********************************************************************
*
*       _Terminal_CheckChannels()
*
*  Function description
*    Verify the RTT channels required by the terminal service.
*/
int _Terminal_CheckChannels(Terminal_State_t *pState) {
    if (pState == NULL) {
        return -1;
    }

    pState->TargetInputEnabled = false;

    if (RTTBridge_CheckUpBufferChannel(pState->Config.channel) != 0) {
        Log_Error("Terminal: RTT up-buffer channel %u is not configured\n",
                  pState->Config.channel);
        return -1;
    }
    if (RTTBridge_CheckDownBufferChannel(pState->Config.channel) != 0) {
        Log_Warn("Terminal: RTT down-buffer channel %u is not configured; "
                 "target input is disabled\n",
                 pState->Config.channel);
        return 0;
    }

    pState->TargetInputEnabled = true;
    return 0;
}

/*********************************************************************
*
*       _Terminal_ReadTargetData()
*
*  Function description
*    Read target output from the owned upstream path.
*/
int _Terminal_ReadTargetData(Terminal_State_t *pState, char *buffer, size_t buffer_size) {
    if ((pState == NULL) || (buffer == NULL) || (buffer_size == 0u)) {
        return -1;
    }

    if (pState->CoreConsumerRegistered) {
        return CoreLogRecorder_ReadChannel(pState->Config.channel, buffer, buffer_size);
    }

    return RTTBridge_ReadUpBufferNoLock(pState->Config.channel, buffer, buffer_size);
}


/*************************** End of file ****************************/
