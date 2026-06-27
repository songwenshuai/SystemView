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
File    : TerminalService.c
Purpose : Terminal public service lifecycle
---------------------------END-OF-HEADER------------------------------
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Terminal_internal.h"
#include "Log.h"

/*********************************************************************
*
*       Terminal_Init()
*
*  Function description
*    Initialize the Terminal service with the specified configuration.
*
*  Parameters
*    pConfig  Pointer to configuration structure.
*
*  Return value
*    0   Success.
*   -1   Invalid configuration or already initialized.
*/
int Terminal_Init(Terminal_Config_t *pConfig) {
    int Ret;
    size_t QueueSize;

    if (pConfig == NULL) {
        return -1;
    }
    if (_terminal_state.Initialized || _terminal_state.Running ||
        _terminal_state.LockInitialized || _terminal_state.ServiceThreadStarted ||
        _terminal_state.DrainThreadStarted) {
        return -1;
    }

    memset(&_terminal_state, 0, sizeof(_terminal_state));
    memcpy(&_terminal_state.Config, pConfig, sizeof(Terminal_Config_t));

    _terminal_state.hListener = SYS_SOCKET_INVALID_HANDLE;
    _terminal_state.hClient   = SYS_SOCKET_INVALID_HANDLE;

    Ret = SYS_MutexInit(&_terminal_state.Lock);
    if (Ret != 0) {
        Log_Error("Terminal: failed to initialize mutex: %d\n", Ret);
        return -1;
    }
    _terminal_state.LockInitialized = true;

    if (pConfig->enabled && !pConfig->console_mode) {
        QueueSize = _Terminal_GetNetworkQueueSize();
        if (QueueSize < TERMINAL_SEND_BUF_SIZE) {
            Log_Error("Terminal: network queue size must be at least %u bytes\n",
                      (unsigned)TERMINAL_SEND_BUF_SIZE);
            SYS_MutexDestroy(&_terminal_state.Lock);
            memset(&_terminal_state, 0, sizeof(_terminal_state));
            return -1;
        }
        _terminal_state.pNetworkQueue = (char *)malloc(QueueSize);
        if (_terminal_state.pNetworkQueue == NULL) {
            Log_Error("Terminal: failed to allocate network queue (%zu bytes)\n",
                      QueueSize);
            SYS_MutexDestroy(&_terminal_state.Lock);
            memset(&_terminal_state, 0, sizeof(_terminal_state));
            return -1;
        }
        ByteQueue_Init(&_terminal_state.NetworkQueue,
                       _terminal_state.pNetworkQueue,
                       QueueSize);
    }

    Log_Print("Terminal service initialized: port=%u, channel=%u, enabled=%s, network_queue=%zu\n",
              pConfig->port,
              pConfig->channel,
              pConfig->enabled ? "yes" : "no",
              ByteQueue_GetCapacity(&_terminal_state.NetworkQueue));

    _terminal_state.Initialized = true;
    return 0;
}

/*********************************************************************
*
*       Terminal_Start()
*
*  Function description
*    Start the Terminal service threads.
*
*  Return value
*    0   Success.
*   -1   Failed to start service.
*/
int Terminal_Start(void) {
    Terminal_State_t *pState = &_terminal_state;
    int               Result;

    if (!pState->Initialized || pState->FatalError) {
        return -1;
    }

    if (!pState->Config.enabled) {
        Log_Print("Terminal service is disabled\n");
        return 0;
    }

    if (_Terminal_CheckChannels(pState) != 0) {
        return -1;
    }

    if (_Terminal_RegisterCoreConsumer(pState) != 0) {
        return -1;
    }

    //
    // Console mode: use stdin/stdout instead of TCP socket
    //
    if (pState->Config.console_mode) {
        //
        // Set terminal to raw mode
        //
        Result = _Console_SetRawMode();
        if (Result < 0) {
            Log_Error("Terminal: failed to set console raw mode\n");
            _Terminal_UnregisterCoreConsumer(pState);
            return -1;
        }
        //
        // Start console service thread
        //
        _Terminal_SetRunning(pState, true);
        Result = SYS_createThread(_Terminal_ConsoleThread, NULL, &pState->ServiceThread);
        if (Result < 0) {
            Log_Error("Terminal: failed to create console service thread\n");
            _Console_RestoreMode();
            _Terminal_SetRunning(pState, false);
            _Terminal_UnregisterCoreConsumer(pState);
            return -1;
        }
        pState->ServiceThreadStarted = true;

        Log_Print("Terminal console service started on channel %u\n", pState->Config.channel);
        return 0;
    }

    //
    // TCP mode: create listening socket
    //
    pState->hListener = SYS_SOCKET_OpenTCP();
    if (pState->hListener == SYS_SOCKET_INVALID_HANDLE) {
        Log_Error("Terminal: failed to open TCP socket\n");
        _Terminal_UnregisterCoreConsumer(pState);
        return -1;
    }

    Result = SYS_SOCKET_ListenAtTCPAddr(pState->hListener, SYS_SOCKET_IP_ADDR_ANY,
                                        pState->Config.port, 1);
    if (Result < 0) {
        Log_Error("Terminal: failed to listen on port %u\n", pState->Config.port);
        SYS_SOCKET_Close(pState->hListener);
        pState->hListener = SYS_SOCKET_INVALID_HANDLE;
        _Terminal_UnregisterCoreConsumer(pState);
        return -1;
    }
    //
    // Start drain and service threads
    //
    _Terminal_SetRunning(pState, true);
    Result = SYS_createThread(_Terminal_DrainThread, NULL, &pState->DrainThread);
    if (Result < 0) {
        Log_Error("Terminal: failed to create drain thread\n");
        SYS_SOCKET_Close(pState->hListener);
        pState->hListener = SYS_SOCKET_INVALID_HANDLE;
        _Terminal_SetRunning(pState, false);
        _Terminal_UnregisterCoreConsumer(pState);
        return -1;
    }
    pState->DrainThreadStarted = true;
    Log_Print("Terminal network queue: %zu bytes, network client failures close the client\n",
              ByteQueue_GetCapacity(&pState->NetworkQueue));

    Result = SYS_createThread(_Terminal_ServiceThread, NULL, &pState->ServiceThread);
    if (Result < 0) {
        Log_Error("Terminal: failed to create service thread\n");
        _Terminal_SetRunning(pState, false);
        if (pState->DrainThreadStarted) {
            SYS_WaitThreadTerm(pState->DrainThread);
            pState->DrainThreadStarted = false;
        }
        SYS_SOCKET_Close(pState->hListener);
        pState->hListener = SYS_SOCKET_INVALID_HANDLE;
        _Terminal_UnregisterCoreConsumer(pState);
        return -1;
    }
    pState->ServiceThreadStarted = true;

    Log_Print("Terminal service started on port %u\n", pState->Config.port);
    return 0;
}

/*********************************************************************
*
*       Terminal_Stop()
*
*  Function description
*    Stop the Terminal service and cleanup resources.
*/
void Terminal_Stop(void) {
    Terminal_State_t *pState = &_terminal_state;
    bool              Running;

    Running = _Terminal_IsRunning(pState);
    if (!pState->Initialized && !Running) {
        return;
    }

    if (Running) {
        Log_Print("Terminal: stopping service\n");
        _Terminal_SetRunning(pState, false);
    }

    //
    // Console mode: restore terminal settings
    //
    if (pState->Config.console_mode) {
        //
        // Wait for thread to exit first
        //
        if (pState->ServiceThreadStarted) {
            SYS_WaitThreadTerm(pState->ServiceThread);
            pState->ServiceThreadStarted = false;
        }
        //
        // Restore terminal to original mode
        //
        _Console_RestoreMode();
    } else {
        //
        // TCP mode: wait for the worker to stop before closing sockets
        //
        if (pState->ServiceThreadStarted) {
            SYS_WaitThreadTerm(pState->ServiceThread);
            pState->ServiceThreadStarted = false;
        }
        if (pState->DrainThreadStarted) {
            SYS_WaitThreadTerm(pState->DrainThread);
            pState->DrainThreadStarted = false;
        }
        _Terminal_CloseClient(pState);
        if (pState->hListener != SYS_SOCKET_INVALID_HANDLE) {
            SYS_SOCKET_Close(pState->hListener);
            pState->hListener = SYS_SOCKET_INVALID_HANDLE;
        }
    }

    _Terminal_UnregisterCoreConsumer(pState);

    if (pState->LockInitialized) {
        SYS_MutexDestroy(&pState->Lock);
        pState->LockInitialized = false;
    }

    free(pState->pNetworkQueue);
    pState->pNetworkQueue = NULL;
    ByteQueue_Init(&pState->NetworkQueue, NULL, 0u);

    pState->Initialized = false;
    Log_Print("Terminal service stopped\n");
}

/*********************************************************************
*
*       Terminal_Status()
*
*  Function description
*    Print current Terminal service status to stdout.
*/
void Terminal_Status(void) {
    Terminal_State_t *pState = &_terminal_state;

    if (pState->LockInitialized) {
        SYS_MutexLock(&pState->Lock);
    }
    printf("Terminal Service Status:\n");
    printf("  Port:             %u\n", pState->Config.port);
    printf("  Channel:          %u\n", pState->Config.channel);
    printf("  Enabled:          %s\n", pState->Config.enabled ? "yes" : "no");
    printf("  Running:          %s\n", pState->Running ? "yes" : "no");
    printf("  Target Input:     %s\n", pState->TargetInputEnabled ? "enabled" : "disabled");
    printf("  Client Connected: %s\n", pState->hClient != SYS_SOCKET_INVALID_HANDLE ? "yes" : "no");
    printf("  Network Queue:    %zu/%zu bytes\n",
           ByteQueue_GetUsed(&pState->NetworkQueue),
           ByteQueue_GetCapacity(&pState->NetworkQueue));
    printf("  Connections:      %u\n", pState->ConnectionsCount);
    printf("  Bytes Sent:       %u\n", pState->BytesSent);
    printf("  Bytes Received:   %u\n", pState->BytesReceived);
    printf("  Fatal Error:      %s\n", pState->FatalError ? "yes" : "no");
    if (pState->LockInitialized) {
        SYS_MutexUnlock(&pState->Lock);
    }
}

/*********************************************************************
*
*       Terminal_IsEnabled()
*
*  Function description
*    Check if the Terminal service is enabled.
*
*  Return value
*    true   Service is enabled.
*    false  Service is disabled.
*/
bool Terminal_IsEnabled(void) {
    return _terminal_state.Config.enabled;
}

/*********************************************************************
*
*       Terminal_HasFatalError()
*
*  Function description
*    Check whether the Terminal service entered a fatal state.
*
*  Return value
*    true   Fatal error occurred.
*    false  No fatal error recorded.
*/
bool Terminal_HasFatalError(void) {
    return _Terminal_HasFatalError(&_terminal_state);
}

/*************************** End of file ****************************/
