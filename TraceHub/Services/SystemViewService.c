/*********************************************************************
*                             CineLogic                              *
*                      RTT Trace and Debug Bridge                    *
**********************************************************************
----------------------------------------------------------------------
File    : SystemViewService.c
Purpose : SystemView public service lifecycle
---------------------------END-OF-HEADER------------------------------
*/

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "SystemView_internal.h"
#include "Log.h"
#include "RTTBridge.h"

/*********************************************************************
*
*       SystemView_Init()
*
*  Function description
*    Initialize the SystemView service with the specified configuration.
*
*  Parameters
*    pConfig  Pointer to configuration structure.
*
*  Return value
*    0   Success.
*   -1   Invalid configuration or already initialized.
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

    Ret = SYS_MutexInit(&_sysview_state.Lock);
    if (Ret != 0) {
        Log_Error("SystemView: failed to initialize mutex: %d\n", Ret);
        return -1;
    }
    _sysview_state.LockInitialized = true;

    if (pConfig->enabled && pConfig->network_enabled) {
        QueueSize = _SystemView_GetNetworkQueueSize();
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
        ByteQueue_Init(&_sysview_state.NetworkQueue,
                       _sysview_state.pNetworkQueue,
                       QueueSize);
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
              ByteQueue_GetCapacity(&_sysview_state.NetworkQueue));

    _sysview_state.Initialized = true;
    return 0;
}

/*********************************************************************
*
*       SystemView_Start()
*
*  Function description
*    Start the recording thread and optional network service thread.
*
*  Return value
*    0   Success.
*   -1   Failed to start service.
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
                  ByteQueue_GetCapacity(&pState->NetworkQueue));
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
*
*  Function description
*    Stop the SystemView service and cleanup resources.
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
    ByteQueue_Init(&pState->NetworkQueue, NULL, 0u);

    pState->Initialized = false;
    Log_Print("SystemView service stopped\n");
}

/*********************************************************************
*
*       SystemView_Status()
*
*  Function description
*    Print current SystemView service status to stdout.
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
    printf("  Network Queue:    %zu/%zu bytes\n",
           ByteQueue_GetUsed(&pState->NetworkQueue),
           ByteQueue_GetCapacity(&pState->NetworkQueue));
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
*
*  Function description
*    Check if the SystemView service is enabled.
*
*  Return value
*    true   Service is enabled.
*    false  Service is disabled.
*/
bool SystemView_IsEnabled(void) {
    return _sysview_state.Config.enabled;
}

/*********************************************************************
*
*       SystemView_HasFatalError()
*
*  Function description
*    Check whether the SystemView service entered a fatal state.
*
*  Return value
*    true   Fatal error occurred.
*    false  No fatal error recorded.
*/
bool SystemView_HasFatalError(void) {
    return _SystemView_HasFatalError(&_sysview_state);
}

/*************************** End of file ****************************/
