/*********************************************************************
*                                                                    *
*                Copyright (C) 2023 xrTest Inc.                      *
*                      All rights reserved                           *
*                                                                    *
**********************************************************************
----------------------------------------------------------------------
File    : SystemView.c
Purpose : SystemView/TRACE service for a configured RTT channel
Author  : songwenshuai <songwenshuai@gmail.com>
---------------------------END-OF-HEADER------------------------------
*/

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "SystemView.h"
#include "RTTBridge.h"
#include "Socket.h"
#include "SYS.h"
#include "Log.h"

/*********************************************************************
*
*       Defines, fixed
*
**********************************************************************
*/

/*********************************************************************
*
*       SYSVIEW_SEND_BUF_SIZE
*  Size of send buffer for RTT to socket data transfer.
*
*/
#define SYSVIEW_SEND_BUF_SIZE       8192

/*********************************************************************
*
*       SYSVIEW_RECV_BUF_SIZE
*  Size of receive buffer for socket to RTT data transfer.
*
*/
#define SYSVIEW_RECV_BUF_SIZE       8192

/*********************************************************************
*
*       SYSVIEW_IDLE_DELAY_MS
*  Maximum delay after which available data from RTT buffer is sent.
*
*/
#define SYSVIEW_IDLE_DELAY_MS       20

/*********************************************************************
*
*       SYSVIEW_HANDSHAKE_TIMEOUT_MS
*  Timeout for SystemView handshake in milliseconds.
*
*/
#define SYSVIEW_HANDSHAKE_TIMEOUT_MS    5000

/*********************************************************************
*
*       SYSVIEW_DEFAULT_NETWORK_QUEUE_SIZE
*  Target trace bytes buffered for network clients.
*
*/
#define SYSVIEW_DEFAULT_NETWORK_QUEUE_SIZE      (1024u * 1024u)

/*********************************************************************
*
*       SYSVIEW_RECORD_FILE_EXTENSION
*  SEGGER SystemView binary recording file extension.
*
*/
#define SYSVIEW_RECORD_FILE_EXTENSION   "SVDat"

/*********************************************************************
*
*       SYSVIEW_RECORD_FLUSH_INTERVAL_MS
*  Maximum time to keep recorded bytes in stdio buffers.
*
*/
#define SYSVIEW_RECORD_FLUSH_INTERVAL_MS  100

/*********************************************************************
*
*       SYSVIEW_RECORD_FLUSH_THRESHOLD
*  Recorded byte threshold that triggers an immediate flush.
*
*/
#define SYSVIEW_RECORD_FLUSH_THRESHOLD    (64u * 1024u)

/*********************************************************************
*
*       SYSVIEW_RECOVERY_DELAY_MS
*  Delay between recoverable RTT access retries.
*
*/
#define SYSVIEW_RECOVERY_DELAY_MS       100

/*********************************************************************
*
*       Types
*
**********************************************************************
*/

/*********************************************************************
*
*       SystemView_State_t
*
*  Description
*    Runtime state of the SystemView service.
*/
typedef struct {
    // Configuration
    SystemView_Config_t Config;

    // Runtime state
    bool                Initialized;
    bool                Running;
    bool                FatalError;
    bool                RecordFileError;
    bool                LockInitialized;
    SYS_SOCKET_HANDLE   hListener;
    SYS_SOCKET_HANDLE   hClient;
    SYS_Thread          ServiceThread;
    SYS_Thread          RecordThread;
    bool                ServiceThreadStarted;
    bool                RecordThreadStarted;
    SYS_Mutex           Lock;
    bool                HandshakeDone;
    bool                ClientDisconnectRequested;
    bool                RTTRecovering;

    // Record file (internally managed)
    FILE               *record_file;

    // Transmission state management for partial send/receive
    int                 SendNumBytes;
    int                 WriteNumBytes;
    char               *pSendBuf;
    char               *pWriteBuf;
    char                acSendBuf[SYSVIEW_SEND_BUF_SIZE];
    char                acWriteBuf[SYSVIEW_RECV_BUF_SIZE];
    char               *pNetworkQueue;
    size_t              NetworkQueueSize;
    size_t              NetworkQueueRead;
    size_t              NetworkQueueUsed;

    // Statistics
    unsigned            BytesSent;
    unsigned            BytesReceived;
    unsigned            ConnectionsCount;
} SystemView_State_t;

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

static SystemView_State_t _sysview_state;

/*********************************************************************
*
*       Static functions
*
**********************************************************************
*/

/*********************************************************************
*
*       _SystemView_GetConfiguredQueueSize()
*
*  Function description
*    Return the configured network queue capacity.
*/
static size_t _SystemView_GetConfiguredQueueSize(const SystemView_Config_t *pConfig) {
    if (pConfig == NULL || pConfig->network_queue_size == 0u) {
        return SYSVIEW_DEFAULT_NETWORK_QUEUE_SIZE;
    }
    return pConfig->network_queue_size;
}

/*********************************************************************
*
*       _SystemView_IsRunning()
*
*  Function description
*    Read the service running state through the module lock.
*/
static bool _SystemView_IsRunning(SystemView_State_t *pState) {
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
static void _SystemView_SetRunning(SystemView_State_t *pState, bool running) {
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
static bool _SystemView_HasFatalError(SystemView_State_t *pState) {
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
*       _SystemView_ReportRecordFileError()
*
*  Function description
*    Mark record file failure as fatal and stop the service.
*/
static void _SystemView_ReportRecordFileError(SystemView_State_t *pState,
                                              const char *operation,
                                              int saved_errno) {
    bool should_report;

    if (pState == NULL) {
        return;
    }

    if (operation == NULL) {
        operation = "operation";
    }

    should_report = false;
    if (pState->LockInitialized) {
        SYS_MutexLock(&pState->Lock);
    }
    if (!pState->RecordFileError) {
        pState->RecordFileError = true;
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
        if (saved_errno != 0) {
            Log_Error("SystemView: record file %s failed: %s\n",
                      operation,
                      strerror(saved_errno));
        } else {
            Log_Error("SystemView: record file %s failed\n", operation);
        }
    }
}

/*********************************************************************
*
*       _SystemView_FlushRecordFile()
*
*  Function description
*    Flush the SystemView record file and report persistence errors.
*/
static int _SystemView_FlushRecordFile(SystemView_State_t *pState, FILE *record_file) {
    if (record_file == NULL) {
        return 0;
    }

    errno = 0;
    if (fflush(record_file) != 0) {
        _SystemView_ReportRecordFileError(pState, "flush", errno);
        return -1;
    }

    return 0;
}

/*********************************************************************
*
*       _SystemView_ReportFatalServiceError()
*
*  Function description
*    Mark an unrecoverable SystemView service failure and stop the service.
*/
static void _SystemView_ReportFatalServiceError(SystemView_State_t *pState,
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
*       _SystemView_ClearNetworkQueueLocked()
*
*  Function description
*    Clear queued live network trace bytes. Caller must hold pState->Lock.
*/
static void _SystemView_ClearNetworkQueueLocked(SystemView_State_t *pState) {
    if (pState == NULL) {
        return;
    }

    pState->NetworkQueueRead = 0u;
    pState->NetworkQueueUsed = 0u;
}

/*********************************************************************
*
*       _SystemView_RequestNetworkStreamResetLocked()
*
*  Function description
*    Drop live network stream state after target RTT continuity is lost.
*    Caller must hold pState->Lock.
*/
static void _SystemView_RequestNetworkStreamResetLocked(SystemView_State_t *pState) {
    if (pState == NULL || !pState->Config.network_enabled) {
        return;
    }

    _SystemView_ClearNetworkQueueLocked(pState);
    pState->ClientDisconnectRequested = true;
}

/*********************************************************************
*
*       _SystemView_ReportRecoverableRTTError()
*
*  Function description
*    Mark RTT access as temporarily unavailable without entering fatal state.
*/
static void _SystemView_ReportRecoverableRTTError(SystemView_State_t *pState,
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
*       _SystemView_ReportRTTRecovered()
*
*  Function description
*    Report transition from RTT recovery wait back to normal operation.
*/
static void _SystemView_ReportRTTRecovered(SystemView_State_t *pState) {
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
static int _SystemView_CheckChannels(SystemView_State_t *pState) {
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
static SYS_SOCKET_HANDLE _SystemView_GetClient(SystemView_State_t *pState) {
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
static void _SystemView_ResetConnectionStateLocked(SystemView_State_t *pState) {
    pState->SendNumBytes  = 0;
    pState->WriteNumBytes = 0;
    pState->pSendBuf      = NULL;
    pState->pWriteBuf     = NULL;
}

/*********************************************************************
*
*       _SystemView_SetConnectedClient()
*
*  Function description
*    Publish a new client after the SystemView handshake completes.
*/
static unsigned _SystemView_SetConnectedClient(SystemView_State_t *pState, SYS_SOCKET_HANDLE hClient) {
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
*/
static void _SystemView_AddBytesSent(SystemView_State_t *pState, unsigned NumBytes) {
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
*/
static void _SystemView_AddBytesReceived(SystemView_State_t *pState, unsigned NumBytes) {
    if ((pState == NULL) || (NumBytes == 0u)) {
        return;
    }

    SYS_MutexLock(&pState->Lock);
    pState->BytesReceived += NumBytes;
    SYS_MutexUnlock(&pState->Lock);
}

/*********************************************************************
*
*       _SystemView_PerformHandshake()
*
*  Function description
*    Perform SystemView handshake protocol with connected client.
*
*    SystemView protocol:
*    1. Client sends 32-byte Hello message
*    2. Server responds with 32-byte Hello containing version info
*
*  Parameters
*    pState - SystemView service state
*    hSock  - Socket handle for the connection
*
*  Return value
*    true  : Handshake successful
*    false : Handshake failed
*/
static bool _SystemView_PerformHandshake(SystemView_State_t *pState, SYS_SOCKET_HANDLE hSock) {
    unsigned char acHelloBuf[SYSVIEW_HELLO_SIZE];
    unsigned char acHelloResponse[SYSVIEW_HELLO_SIZE];
    int           BytesRead = 0;
    int           BytesSent = 0;
    int           Result;
    int           TimeoutRemaining = SYSVIEW_HANDSHAKE_TIMEOUT_MS;
    //
    // Read Hello message from SystemView client (32 bytes)
    //
    while ((BytesRead < SYSVIEW_HELLO_SIZE) &&
           (TimeoutRemaining > 0) &&
           _SystemView_IsRunning(pState)) {
        Result = SYS_SOCKET_IsReadable(hSock, 100);
        if (Result < 0) {
            Log_Warn("SystemView handshake: socket error during read wait\n");
            return false;
        }
        if (Result == 0) {
            TimeoutRemaining -= 100;
            continue;
        }

        Result = SYS_SOCKET_Receive(hSock, acHelloBuf + BytesRead,
                                    SYSVIEW_HELLO_SIZE - BytesRead);
        if (Result < 0) {
            if (Result != SYS_SOCKET_ERR_WOULDBLOCK) {
                Log_Warn("SystemView handshake: failed to read Hello message: %d\n", Result);
                return false;
            }
            continue;
        }
        if (Result == 0) {
            Log_Warn("SystemView handshake: connection closed by client\n");
            return false;
        }

        BytesRead += Result;
        TimeoutRemaining = SYSVIEW_HANDSHAKE_TIMEOUT_MS;
    }

    if (BytesRead < SYSVIEW_HELLO_SIZE) {
        if (_SystemView_IsRunning(pState)) {
            Log_Warn("SystemView handshake: timeout reading Hello message (got %d/%d bytes)\n",
                     BytesRead, SYSVIEW_HELLO_SIZE);
        }
        return false;
    }

    if (!SystemView_HelloHasValidPrefix(acHelloBuf, sizeof(acHelloBuf))) {
        Log_Warn("SystemView handshake: invalid Hello prefix\n");
        return false;
    }
    if (!SystemView_BuildHelloMessage(acHelloResponse, sizeof(acHelloResponse))) {
        Log_Warn("SystemView handshake: failed to build Hello response\n");
        return false;
    }

    Log_Print("SystemView handshake: received Hello message (%d bytes)\n", BytesRead);
    //
    // Send our Hello response
    //
    TimeoutRemaining = SYSVIEW_HANDSHAKE_TIMEOUT_MS;
    while ((BytesSent < SYSVIEW_HELLO_SIZE) &&
           (TimeoutRemaining > 0) &&
           _SystemView_IsRunning(pState)) {
        Result = SYS_SOCKET_IsWriteable(hSock, 100);
        if (Result < 0) {
            Log_Warn("SystemView handshake: socket error during write wait\n");
            return false;
        }
        if (Result == 0) {
            TimeoutRemaining -= 100;
            continue;
        }

        Result = SYS_SOCKET_Send(hSock,
                                 acHelloResponse + BytesSent,
                                 (unsigned)(SYSVIEW_HELLO_SIZE - BytesSent));
        if (Result < 0) {
            if (Result != SYS_SOCKET_ERR_WOULDBLOCK) {
                Log_Warn("SystemView handshake: failed to send Hello response: %d\n", Result);
                return false;
            }
            continue;
        }
        if (Result == 0) {
            Log_Warn("SystemView handshake: connection closed while sending Hello response\n");
            return false;
        }

        BytesSent += Result;
        TimeoutRemaining = SYSVIEW_HANDSHAKE_TIMEOUT_MS;
    }

    if (BytesSent < SYSVIEW_HELLO_SIZE) {
        if (_SystemView_IsRunning(pState)) {
            Log_Warn("SystemView handshake: timeout sending Hello response (sent %d/%d bytes)\n",
                     BytesSent, SYSVIEW_HELLO_SIZE);
        }
        return false;
    }

    Log_Print("SystemView handshake: sent Hello response (%d bytes)\n", BytesSent);
    return true;
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
static int _SystemView_QueueTargetData(SystemView_State_t *pState, const char *data, unsigned num_bytes) {
    size_t free_space;
    size_t write_pos;
    size_t first_chunk;
    bool   queue_overflow;

    if ((pState == NULL) || (data == NULL) || (num_bytes == 0u)) {
        return 0;
    }

    SYS_MutexLock(&pState->Lock);

    if (!pState->Config.network_enabled) {
        SYS_MutexUnlock(&pState->Lock);
        return 0;
    }
    if (pState->pNetworkQueue == NULL || pState->NetworkQueueSize == 0u) {
        SYS_MutexUnlock(&pState->Lock);
        _SystemView_ReportFatalServiceError(pState, "network queue");
        return -1;
    }

    queue_overflow = false;
    free_space = pState->NetworkQueueSize - pState->NetworkQueueUsed;
    if ((size_t)num_bytes > free_space) {
        _SystemView_ClearNetworkQueueLocked(pState);
        pState->ClientDisconnectRequested = true;
        queue_overflow = true;
        free_space = pState->NetworkQueueSize;
    }

    if ((size_t)num_bytes > free_space) {
        SYS_MutexUnlock(&pState->Lock);
        Log_Error("SystemView: target trace batch exceeds network queue capacity "
                  "(requested=%u, capacity=%zu)\n",
                  num_bytes,
                  pState->NetworkQueueSize);
        _SystemView_ReportFatalServiceError(pState, "network queue");
        return -1;
    }

    write_pos = (pState->NetworkQueueRead + pState->NetworkQueueUsed) % pState->NetworkQueueSize;
    first_chunk = pState->NetworkQueueSize - write_pos;
    if (first_chunk > (size_t)num_bytes) {
        first_chunk = (size_t)num_bytes;
    }

    memcpy(&pState->pNetworkQueue[write_pos], data, first_chunk);
    if (first_chunk < (size_t)num_bytes) {
        memcpy(&pState->pNetworkQueue[0], data + first_chunk, (size_t)num_bytes - first_chunk);
    }
    pState->NetworkQueueUsed += num_bytes;

    SYS_MutexUnlock(&pState->Lock);

    if (queue_overflow) {
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
static unsigned _SystemView_DequeueTargetData(SystemView_State_t *pState, char *buffer, unsigned buffer_size) {
    size_t   num_bytes;
    size_t   first_chunk;
    unsigned result;

    if ((pState == NULL) || (buffer == NULL) || (buffer_size == 0u)) {
        return 0u;
    }
    if (pState->pNetworkQueue == NULL || pState->NetworkQueueSize == 0u) {
        return 0u;
    }

    SYS_MutexLock(&pState->Lock);

    num_bytes = pState->NetworkQueueUsed;
    if (num_bytes > (size_t)buffer_size) {
        num_bytes = (size_t)buffer_size;
    }
    if (num_bytes == 0u) {
        SYS_MutexUnlock(&pState->Lock);
        return 0u;
    }

    first_chunk = pState->NetworkQueueSize - pState->NetworkQueueRead;
    if (first_chunk > num_bytes) {
        first_chunk = num_bytes;
    }

    memcpy(buffer, &pState->pNetworkQueue[pState->NetworkQueueRead], first_chunk);
    if (first_chunk < num_bytes) {
        memcpy(buffer + first_chunk, &pState->pNetworkQueue[0], num_bytes - first_chunk);
    }

    pState->NetworkQueueRead = (pState->NetworkQueueRead + num_bytes) % pState->NetworkQueueSize;
    pState->NetworkQueueUsed -= num_bytes;
    if (pState->NetworkQueueUsed == 0u) {
        pState->NetworkQueueRead = 0u;
    }

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
static void _SystemView_CloseClient(SystemView_State_t *pState) {
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
static bool _SystemView_TakeClientDisconnectRequest(SystemView_State_t *pState) {
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
*       _SystemView_CloseClientForNetworkError()
*
*  Function description
*    Close the current network client after a recoverable socket error.
*/
static void _SystemView_CloseClientForNetworkError(SystemView_State_t *pState,
                                                   const char *operation) {
    if (operation == NULL) {
        operation = "connection";
    }

    Log_Warn("SystemView: network %s; closing current client\n", operation);
    _SystemView_CloseClient(pState);
}

/*********************************************************************
*
*       _SystemView_RecordingThread()
*
*  Function description
*    Recording thread for SystemView data.
*    Reads RTT data, records to file, and feeds the network queue.
*
*  Parameters
*    pArg - Unused parameter (required by thread API)
*/
static void _SystemView_RecordingThread(void *pArg) {
    (void)pArg;

    SystemView_State_t *pState = &_sysview_state;
    uintptr_t           Address;
    int                 NumBytes;
    FILE               *record_file;
    char                Buffer[SYSVIEW_SEND_BUF_SIZE];
    unsigned            Now;
    unsigned            LastRecordFlushTick;
    size_t              Written;
    size_t              PendingRecordBytes;

    Log_Print("SystemView recording thread started, channel %u\n",
              pState->Config.channel);

    LastRecordFlushTick = SYS_GetTickCount();
    PendingRecordBytes = 0u;

    while (_SystemView_IsRunning(pState)) {
        record_file = pState->record_file;
        if (RTTBridge_GetValidatedRTTRegion(&Address, NULL) != 0) {
            _SystemView_ReportRecoverableRTTError(pState, "region validation");
            SYS_Sleep(SYSVIEW_RECOVERY_DELAY_MS);
            continue;
        }

        //
        // Read data from RTT buffer
        //
        NumBytes = RTTBridge_ReadUpBufferNoLock(pState->Config.channel, Buffer, sizeof(Buffer));

        if (NumBytes >= 0) {
            _SystemView_ReportRTTRecovered(pState);
        }
        if (NumBytes > 0) {
            //
            // Record to file if available
            //
            if (record_file != NULL) {
                errno = 0;
                Written = fwrite(Buffer, 1, (size_t)NumBytes, record_file);
                if (Written != (size_t)NumBytes) {
                    _SystemView_ReportRecordFileError(pState, "write", errno);
                    continue;
                }
                PendingRecordBytes += Written;
                Now = SYS_GetTickCount();
                if ((PendingRecordBytes >= SYSVIEW_RECORD_FLUSH_THRESHOLD) ||
                    ((unsigned)(Now - LastRecordFlushTick) >= SYSVIEW_RECORD_FLUSH_INTERVAL_MS)) {
                    if (_SystemView_FlushRecordFile(pState, record_file) != 0) {
                        break;
                    }
                    LastRecordFlushTick = Now;
                    PendingRecordBytes = 0u;
                }
            }
            if (_SystemView_QueueTargetData(pState, Buffer, (unsigned)NumBytes) != 0) {
                break;
            }
        } else if (NumBytes < 0) {
            _SystemView_ReportRecoverableRTTError(pState, "up-buffer read");
            SYS_Sleep(SYSVIEW_RECOVERY_DELAY_MS);
            continue;
        } else {
            //
            // No data available, sleep to reduce CPU usage
            //
            SYS_Sleep(SYSVIEW_IDLE_DELAY_MS);
        }
    }

    record_file = pState->record_file;
    if ((record_file != NULL) && (PendingRecordBytes > 0u)) {
        (void)_SystemView_FlushRecordFile(pState, record_file);
    }

    Log_Print("SystemView recording thread stopped\n");
}

/*********************************************************************
*
*       _SystemView_ServiceThread()
*
*  Function description
*    Main service thread handling accept and poll operations.
*    Accepts new connections and handles RTT data transfer.
*
*  Parameters
*    pArg - Unused parameter (required by thread API)
*/
static void _SystemView_ServiceThread(void *pArg) {
    (void)pArg;

    SystemView_State_t *pState = &_sysview_state;
    uintptr_t           Address;
    SYS_SOCKET_HANDLE   hClient;
    int                 Result;
    int                 NumBytes;
    unsigned            ConnectionsCount;

    Log_Print("SystemView service thread started on port %u, channel %u\n",
              pState->Config.port, pState->Config.channel);

    while (_SystemView_IsRunning(pState)) {
        if (RTTBridge_GetValidatedRTTRegion(&Address, NULL) != 0) {
            _SystemView_ReportRecoverableRTTError(pState, "region validation");
            SYS_Sleep(SYSVIEW_RECOVERY_DELAY_MS);
            continue;
        }
        if (RTTBridge_CheckDownBufferChannel(pState->Config.channel) != 0) {
            _SystemView_ReportRecoverableRTTError(pState, "down-buffer validation");
            SYS_Sleep(SYSVIEW_RECOVERY_DELAY_MS);
            continue;
        }
        _SystemView_ReportRTTRecovered(pState);

        if (_SystemView_TakeClientDisconnectRequest(pState)) {
            _SystemView_CloseClient(pState);
            continue;
        }

        //
        // Check if we have a connected client
        //
        hClient = _SystemView_GetClient(pState);
        if (hClient != SYS_SOCKET_INVALID_HANDLE) {
            Result = SYS_SOCKET_IsWriteable(hClient, 10);
            if (Result == 0) {
                // Timeout, continue polling
                continue;
            } else if (Result < 0) {
                // Error, close connection
                Log_Warn("SystemView: socket connection error\n");
                _SystemView_CloseClient(pState);
                continue;
            }
        } else {
            //
            // Wait for new connection
            //
            Log_Print("SystemView: waiting for new connection on port %u\n", pState->Config.port);
            hClient = SYS_SOCKET_AcceptEx(pState->hListener, SYSVIEW_IDLE_DELAY_MS);

            if (hClient < 0 && hClient != SYS_SOCKET_ERR_ACCEPT_TIMEOUT) {
                Log_Warn("SystemView: failed to accept connection\n");
                SYS_Sleep(1000);
                continue;
            }

            if (hClient == SYS_SOCKET_ERR_ACCEPT_TIMEOUT) {
                continue;
            }

            Result = SYS_SOCKET_IsReady(hClient);
            if (Result != 1) {
                Log_Warn("SystemView: failed to connect\n");
                SYS_SOCKET_Close(hClient);
                SYS_Sleep(1000);
                continue;
            }

            SYS_SOCKET_EnableKeepalive(hClient);
            //
            // Perform SystemView handshake (blocking socket for handshake)
            //
            if (!_SystemView_PerformHandshake(pState, hClient)) {
                Log_Warn("SystemView: handshake failed, closing connection\n");
                SYS_SOCKET_Close(hClient);
                continue;
            }
            //
            // Switch to non-blocking mode after handshake
            //
            SYS_SOCKET_SetNonBlocking(hClient);
            //
            // Publish the negotiated client connection
            //
            ConnectionsCount = _SystemView_SetConnectedClient(pState, hClient);

            Log_Print("SystemView: new client connected (total: %u)\n", ConnectionsCount);
        }

        //
        // Receive new data from socket only if previous data has been fully written to RTT
        //
        if (pState->WriteNumBytes == 0) {
            Result = SYS_SOCKET_IsReadable(hClient, 0);
            if (Result == 1) {
                Result = SYS_SOCKET_Receive(hClient, pState->acWriteBuf, sizeof(pState->acWriteBuf));
                if (Result < 0) {
                    if (Result != SYS_SOCKET_ERR_WOULDBLOCK) {
                        _SystemView_CloseClientForNetworkError(pState, "socket receive failed");
                        continue;
                    }
                } else if (Result == 0) {
                    _SystemView_CloseClientForNetworkError(pState, "client disconnected");
                    continue;
                } else if (Result > 0) {
                    pState->WriteNumBytes = Result;
                    pState->pWriteBuf     = pState->acWriteBuf;
                    _SystemView_AddBytesReceived(pState, (unsigned)Result);
                }
            }
        }

        //
        // Write pending data to RTT buffer
        //
        if (pState->WriteNumBytes > 0) {
            NumBytes = RTTBridge_WriteDownBufferNoLock(pState->Config.channel,
                                                       pState->pWriteBuf, pState->WriteNumBytes);
            if (NumBytes < 0) {
                _SystemView_ReportRecoverableRTTError(pState, "down-buffer write");
                SYS_Sleep(SYSVIEW_RECOVERY_DELAY_MS);
                continue;
            }
            if (NumBytes > 0) {
                pState->pWriteBuf     += NumBytes;
                pState->WriteNumBytes -= NumBytes;
            }
        }

        //
        // Load queued target trace data only if previous data has been fully sent
        //
        if (pState->SendNumBytes == 0) {
            NumBytes = (int)_SystemView_DequeueTargetData(pState,
                                                          pState->acSendBuf,
                                                          sizeof(pState->acSendBuf));
            if (NumBytes > 0) {
                pState->SendNumBytes = NumBytes;
                pState->pSendBuf     = pState->acSendBuf;
            }
        }

        //
        // Send pending data to socket
        //
        if (pState->SendNumBytes > 0) {
            Result = SYS_SOCKET_Send(hClient, pState->pSendBuf, pState->SendNumBytes);
            if (Result < 0) {
                if (Result != SYS_SOCKET_ERR_WOULDBLOCK) {
                    _SystemView_CloseClientForNetworkError(pState, "socket send failed");
                    continue;
                }
            } else if (Result > 0) {
                pState->pSendBuf     += Result;
                pState->SendNumBytes -= Result;
                _SystemView_AddBytesSent(pState, (unsigned)Result);
            }
        }

        SYS_Sleep(1);
        RTTBridge_IncrementPolls();
    }

    Log_Print("SystemView service thread exiting\n");
}

/*********************************************************************
*
*       Public functions
*
**********************************************************************
*/

/*********************************************************************
*
*       SystemView_Init()
*/
int SystemView_Init(SystemView_Config_t *pConfig) {
    int Ret;
    size_t QueueSize;

    if (pConfig == NULL) {
        return -1;
    }
    if (_sysview_state.Initialized || _sysview_state.Running ||
        _sysview_state.LockInitialized || _sysview_state.ServiceThreadStarted ||
        _sysview_state.RecordThreadStarted || (_sysview_state.record_file != NULL)) {
        return -1;
    }

    memset(&_sysview_state, 0, sizeof(_sysview_state));
    memcpy(&_sysview_state.Config, pConfig, sizeof(SystemView_Config_t));

    _sysview_state.hListener = SYS_SOCKET_INVALID_HANDLE;
    _sysview_state.hClient   = SYS_SOCKET_INVALID_HANDLE;
    _sysview_state.NetworkQueueSize = _SystemView_GetConfiguredQueueSize(pConfig);

    Ret = SYS_MutexInit(&_sysview_state.Lock);
    if (Ret != 0) {
        Log_Error("SystemView: failed to initialize mutex: %d\n", Ret);
        return -1;
    }
    _sysview_state.LockInitialized = true;

    if (pConfig->enabled && pConfig->network_enabled) {
        QueueSize = _sysview_state.NetworkQueueSize;
        if (QueueSize < SYSVIEW_SEND_BUF_SIZE) {
            Log_Error("SystemView: network queue size must be at least %u bytes\n",
                      (unsigned)SYSVIEW_SEND_BUF_SIZE);
            SYS_MutexDestroy(&_sysview_state.Lock);
            memset(&_sysview_state, 0, sizeof(_sysview_state));
            return -1;
        }
        _sysview_state.pNetworkQueue = (char *)malloc(QueueSize);
        if (_sysview_state.pNetworkQueue == NULL) {
            Log_Error("SystemView: failed to allocate network queue (%zu bytes)\n",
                      QueueSize);
            SYS_MutexDestroy(&_sysview_state.Lock);
            memset(&_sysview_state, 0, sizeof(_sysview_state));
            return -1;
        }
    }

    //
    // Create record file if recording is enabled
    //
    if (pConfig->record_enabled) {
        if (pConfig->record_prefix == NULL) {
            Log_Error("SystemView: record prefix is required when recording is enabled\n");
            free(_sysview_state.pNetworkQueue);
            SYS_MutexDestroy(&_sysview_state.Lock);
            memset(&_sysview_state, 0, sizeof(_sysview_state));
            return -1;
        }
        _sysview_state.record_file = LOG_CreateTimestampedFileEx(pConfig->record_prefix,
                                                                 SYSVIEW_RECORD_FILE_EXTENSION,
                                                                 "wb");
        if (_sysview_state.record_file == NULL) {
            Log_Error("SystemView: failed to create record file\n");
            free(_sysview_state.pNetworkQueue);
            SYS_MutexDestroy(&_sysview_state.Lock);
            memset(&_sysview_state, 0, sizeof(_sysview_state));
            return -1;
        }
        Log_Print("SystemView: SVDat record file created\n");
    }

    Log_Print("SystemView service initialized: port=%u, channel=%u, enabled=%s, network_queue=%zu\n",
              pConfig->port,
              pConfig->channel,
              pConfig->enabled ? "yes" : "no",
              _sysview_state.NetworkQueueSize);

    _sysview_state.Initialized = true;
    return 0;
}

/*********************************************************************
*
*       SystemView_Start()
*/
int SystemView_Start(void) {
    SystemView_State_t *pState = &_sysview_state;
    int                 Result;

    if (!pState->Initialized || pState->FatalError) {
        return -1;
    }

    if (!pState->Config.enabled) {
        Log_Print("SystemView service is disabled\n");
        return 0;
    }
    if (!pState->Config.record_enabled && !pState->Config.network_enabled) {
        Log_Error("SystemView: no record or network mode enabled\n");
        return -1;
    }
    if (_SystemView_CheckChannels(pState) != 0) {
        return -1;
    }

    _SystemView_SetRunning(pState, true);

    if (pState->Config.record_enabled || pState->Config.network_enabled) {
        Result = SYS_createThread(_SystemView_RecordingThread, NULL, &pState->RecordThread);
        if (Result < 0) {
            Log_Error("SystemView: failed to create recording thread\n");
            _SystemView_SetRunning(pState, false);
            return -1;
        }
        pState->RecordThreadStarted = true;
        Log_Print("SystemView recording started (channel %u)\n", pState->Config.channel);
    }

    if (pState->Config.network_enabled) {
        Log_Print("SystemView network queue: %zu bytes, network client failures close the client\n",
                  pState->NetworkQueueSize);
        //
        // Network service mode: create listening socket
        //
        pState->hListener = SYS_SOCKET_OpenTCP();
        if (pState->hListener == SYS_SOCKET_INVALID_HANDLE) {
            Log_Error("SystemView: failed to open TCP socket\n");
            _SystemView_SetRunning(pState, false);
            if (pState->RecordThreadStarted) {
                SYS_WaitThreadTerm(pState->RecordThread);
                pState->RecordThreadStarted = false;
            }
            return -1;
        }

        Result = SYS_SOCKET_ListenAtTCPAddr(pState->hListener, SYS_SOCKET_IP_ADDR_ANY,
                                            pState->Config.port, 1);
        if (Result < 0) {
            Log_Error("SystemView: failed to listen on port %u\n", pState->Config.port);
            SYS_SOCKET_Close(pState->hListener);
            pState->hListener = SYS_SOCKET_INVALID_HANDLE;
            _SystemView_SetRunning(pState, false);
            if (pState->RecordThreadStarted) {
                SYS_WaitThreadTerm(pState->RecordThread);
                pState->RecordThreadStarted = false;
            }
            return -1;
        }
        //
        // Start network service thread
        //
        Result = SYS_createThread(_SystemView_ServiceThread, NULL, &pState->ServiceThread);
        if (Result < 0) {
            Log_Error("SystemView: failed to create service thread\n");
            SYS_SOCKET_Close(pState->hListener);
            pState->hListener = SYS_SOCKET_INVALID_HANDLE;
            _SystemView_SetRunning(pState, false);
            if (pState->RecordThreadStarted) {
                SYS_WaitThreadTerm(pState->RecordThread);
                pState->RecordThreadStarted = false;
            }
            return -1;
        }
        pState->ServiceThreadStarted = true;

        Log_Print("SystemView service started on port %u\n", pState->Config.port);
    }

    return 0;
}

/*********************************************************************
*
*       SystemView_Stop()
*/
void SystemView_Stop(void) {
    SystemView_State_t *pState = &_sysview_state;
    bool                Running;

    Running = _SystemView_IsRunning(pState);
    if (!pState->Initialized && !Running) {
        return;
    }

    if (Running) {
        Log_Print("SystemView: stopping service\n");
        _SystemView_SetRunning(pState, false);
    }
    //
    // Wait for threads to exit
    //
    if (pState->ServiceThreadStarted) {
        SYS_WaitThreadTerm(pState->ServiceThread);
        pState->ServiceThreadStarted = false;
    }
    if (pState->RecordThreadStarted) {
        SYS_WaitThreadTerm(pState->RecordThread);
        pState->RecordThreadStarted = false;
    }
    //
    // Close sockets after worker threads have stopped using them.
    //
    _SystemView_CloseClient(pState);
    if (pState->hListener != SYS_SOCKET_INVALID_HANDLE) {
        SYS_SOCKET_Close(pState->hListener);
        pState->hListener = SYS_SOCKET_INVALID_HANDLE;
    }

    //
    // Close record file if opened
    //
    if (pState->record_file != NULL) {
        FILE *record_file;

        record_file = pState->record_file;
        pState->record_file = NULL;
        errno = 0;
        if (fclose(record_file) != 0) {
            _SystemView_ReportRecordFileError(pState, "close", errno);
        }
    }

    if (pState->LockInitialized) {
        SYS_MutexDestroy(&pState->Lock);
        pState->LockInitialized = false;
    }

    free(pState->pNetworkQueue);
    pState->pNetworkQueue = NULL;
    pState->NetworkQueueSize = 0u;
    pState->NetworkQueueRead = 0u;
    pState->NetworkQueueUsed = 0u;

    pState->Initialized = false;
    Log_Print("SystemView service stopped\n");
}

/*********************************************************************
*
*       SystemView_Status()
*/
void SystemView_Status(void) {
    SystemView_State_t *pState = &_sysview_state;

    if (pState->LockInitialized) {
        SYS_MutexLock(&pState->Lock);
    }
    printf("SystemView Service Status:\n");
    printf("  Port:             %u\n", pState->Config.port);
    printf("  Channel:          %u\n", pState->Config.channel);
    printf("  Enabled:          %s\n", pState->Config.enabled ? "yes" : "no");
    printf("  Running:          %s\n", pState->Running ? "yes" : "no");
    printf("  Fatal Error:      %s\n", pState->FatalError ? "yes" : "no");
    printf("  Record File Error: %s\n", pState->RecordFileError ? "yes" : "no");
    printf("  RTT Recovering:   %s\n", pState->RTTRecovering ? "yes" : "no");
    printf("  Client Connected: %s\n", pState->hClient != SYS_SOCKET_INVALID_HANDLE ? "yes" : "no");
    printf("  Handshake Done:   %s\n", pState->HandshakeDone ? "yes" : "no");
    printf("  Network Queue:    %zu/%zu bytes\n", pState->NetworkQueueUsed, pState->NetworkQueueSize);
    printf("  Connections:      %u\n", pState->ConnectionsCount);
    printf("  Bytes Sent:       %u\n", pState->BytesSent);
    printf("  Bytes Received:   %u\n", pState->BytesReceived);
    if (pState->LockInitialized) {
        SYS_MutexUnlock(&pState->Lock);
    }
}

/*********************************************************************
*
*       SystemView_IsEnabled()
*/
bool SystemView_IsEnabled(void) {
    return _sysview_state.Config.enabled;
}

/*********************************************************************
*
*       SystemView_HasFatalError()
*/
bool SystemView_HasFatalError(void) {
    return _SystemView_HasFatalError(&_sysview_state);
}

/*************************** End of file ****************************/
