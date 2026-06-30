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
File    : TerminalRuntime.c
Purpose : Terminal TCP service and RTT transfer runtime
---------------------------END-OF-HEADER------------------------------
*/

#include <stdint.h>
#include <stdio.h>

#include "Terminal_internal.h"
#include "Log.h"
#include "RTTBridge.h"

/*********************************************************************
*
*       _Terminal_DrainThread()
*
*  Function description
*    TCP-mode RTT output drain thread.
*    Reads target output continuously and feeds the network queue.
*
*  Parameters
*    pArg - Unused parameter (required by thread API)
*/
void _Terminal_DrainThread(void *pArg) {
    (void)pArg;

    Terminal_State_t *pState = &_terminal_state;
    int               NumBytes;
    char              Buffer[TERMINAL_SEND_BUF_SIZE];

    Log_Print("Terminal drain thread started on channel %u\n", pState->Config.channel);

    while (_Terminal_IsRunning(pState)) {
        NumBytes = _Terminal_ReadTargetData(pState, Buffer, sizeof(Buffer));
        if (NumBytes < 0) {
            if (pState->CoreConsumerRegistered) {
                _Terminal_ReportRTTError(pState, "core recorder read");
                break;
            }
            _Terminal_ReportRecoverableRTTError(pState, "target read");
            SYS_Sleep(TERMINAL_RECOVERY_DELAY_MS);
            continue;
        }
        _Terminal_ReportRTTRecovered(pState);
        if (NumBytes > 0) {
            if (_Terminal_QueueTargetData(pState, Buffer, (unsigned)NumBytes) != 0) {
                break;
            }
        } else {
            SYS_Sleep(TERMINAL_IDLE_DELAY_MS);
        }
    }

    Log_Print("Terminal drain thread exiting\n");
}

/*********************************************************************
*
*       _Terminal_CloseClient()
*
*  Function description
*    Close the active client socket and reset per-connection transfer state.
*
*  Parameters
*    pState - Terminal service state
*/
void _Terminal_CloseClient(Terminal_State_t *pState) {
    SYS_SOCKET_HANDLE hClient;

    if (pState == NULL) {
        return;
    }

    SYS_MutexLock(&pState->Lock);
    hClient = pState->hClient;
    pState->hClient = SYS_SOCKET_INVALID_HANDLE;
    pState->ClientDisconnectRequested = false;
    _Terminal_ResetConnectionStateLocked(pState);
    SYS_MutexUnlock(&pState->Lock);

    if (hClient != SYS_SOCKET_INVALID_HANDLE) {
        SYS_SOCKET_Shutdown(hClient, SYS_SOCKET_SHUT_RDWR);
        SYS_SOCKET_Close(hClient);
    }
}

/*********************************************************************
*
*       _Terminal_TakeClientDisconnectRequest()
*
*  Function description
*    Consume a pending request to close the current client.
*/
bool _Terminal_TakeClientDisconnectRequest(Terminal_State_t *pState) {
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
*       _Terminal_CloseClientForNetworkError()
*
*  Function description
*    Close the current network client after a recoverable socket error.
*/
void _Terminal_CloseClientForNetworkError(Terminal_State_t *pState,
                                                 const char *operation) {
    if (operation == NULL) {
        operation = "connection";
    }

    Log_Warn("Terminal: network %s; closing current client\n", operation);
    _Terminal_CloseClient(pState);
}

/*********************************************************************
*
*       _Terminal_ServiceThread()
*
*  Function description
*    Main network service thread.
*    Accepts new connections and handles client input/output.
*
*  Parameters
*    pArg - Unused parameter (required by thread API)
*/
void _Terminal_ServiceThread(void *pArg) {
    (void)pArg;

    Terminal_State_t *pState = &_terminal_state;
    uintptr_t         Address;
    SYS_SOCKET_HANDLE hClient;
    int               Result;
    int               NumBytes;
    char              acTelnetResponse[TERMINAL_RECV_BUF_SIZE];
    unsigned          FilteredLen;
    unsigned          ResponseLen;
    unsigned          ConnectionsCount;

    Log_Print("Terminal service thread started on port %u, channel %u\n",
              pState->Config.port, pState->Config.channel);

    while (_Terminal_IsRunning(pState)) {
        if (RTTBridge_GetValidatedRTTRegion(&Address, NULL) != 0) {
            _Terminal_ReportRecoverableRTTError(pState, "region validation");
            SYS_Sleep(TERMINAL_RECOVERY_DELAY_MS);
            continue;
        }
        _Terminal_ReportRTTRecovered(pState);

        if (_Terminal_TakeClientDisconnectRequest(pState)) {
            _Terminal_CloseClient(pState);
            continue;
        }

        //
        // Check if we have a connected client
        //
        hClient = _Terminal_GetClient(pState);
        if (hClient == SYS_SOCKET_INVALID_HANDLE) {
            //
            // Wait for new connection
            //
            Log_Print("Terminal: waiting for new connection on port %u\n", pState->Config.port);
            Result = SYS_SOCKET_AcceptEx(pState->hListener,
                                         TERMINAL_IDLE_DELAY_MS,
                                         &hClient);

            if (Result == SYS_SOCKET_ERR_ACCEPT_TIMEOUT) {
                continue;
            }

            if (Result != 0) {
                Log_Warn("Terminal: failed to accept connection\n");
                SYS_Sleep(1000);
                continue;
            }

            Result = SYS_SOCKET_IsReady(hClient);
            if (Result != 1) {
                Log_Warn("Terminal: failed to connect\n");
                SYS_SOCKET_Shutdown(hClient, SYS_SOCKET_SHUT_RDWR);
                SYS_SOCKET_Close(hClient);
                SYS_Sleep(1000);
                continue;
            }

            SYS_SOCKET_EnableKeepalive(hClient);
            SYS_SOCKET_SetNonBlocking(hClient);
            //
            // Perform Telnet option negotiation
            //
            Result = TelnetCodec_SendNegotiation(hClient);
            if (Result != 0) {
                SYS_SOCKET_Shutdown(hClient, SYS_SOCKET_SHUT_RDWR);
                SYS_SOCKET_Close(hClient);
                continue;
            }
            //
            // Publish the negotiated client connection
            //
            ConnectionsCount = _Terminal_SetConnectedClient(pState, hClient);

            Log_Print("Terminal: new client connected (total: %u)\n", ConnectionsCount);
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
                        _Terminal_CloseClientForNetworkError(pState, "socket receive failed");
                        continue;
                    }
                } else if (Result == 0) {
                    _Terminal_CloseClientForNetworkError(pState, "client disconnected");
                    continue;
                } else if (Result > 0) {
                    FilteredLen = TelnetCodec_FilterClientData(&pState->TelnetState,
                                                               pState->acWriteBuf,
                                                               (unsigned)Result,
                                                               pState->acWriteBuf,
                                                               sizeof(pState->acWriteBuf),
                                                               acTelnetResponse,
                                                               sizeof(acTelnetResponse),
                                                               &ResponseLen);
                    if (ResponseLen > 0u) {
                        Result = SYS_SOCKET_SendAll(hClient, acTelnetResponse, ResponseLen,
                                                    TELNET_CODEC_RESPONSE_TIMEOUT_MS);
                        if (Result != (int)ResponseLen) {
                            _Terminal_CloseClientForNetworkError(pState, "Telnet response send failed");
                            continue;
                        }
                    }
                    if (FilteredLen > 0u && pState->TargetInputEnabled) {
                        pState->WriteNumBytes = (int)FilteredLen;
                        pState->pWriteBuf     = pState->acWriteBuf;
                        _Terminal_AddBytesReceived(pState, FilteredLen);
                    }
                }
            }
        }

        //
        // Write pending data to RTT buffer
        //
        if (pState->TargetInputEnabled && pState->WriteNumBytes > 0) {
            NumBytes = RTTBridge_WriteDownBufferNoLock(pState->Config.channel,
                                                       pState->pWriteBuf, pState->WriteNumBytes);
            if (NumBytes < 0) {
                _Terminal_ReportRecoverableRTTError(pState, "down-buffer write");
                SYS_Sleep(TERMINAL_RECOVERY_DELAY_MS);
                continue;
            }
            _Terminal_ReportRTTRecovered(pState);
            if (NumBytes > 0) {
                pState->pWriteBuf     += NumBytes;
                pState->WriteNumBytes -= NumBytes;
            }
        }

        //
        // Load queued RTT output only if previous data has been fully sent
        //
        if (pState->SendNumBytes == 0) {
            NumBytes = (int)_Terminal_DequeueTargetData(pState,
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
                    _Terminal_CloseClientForNetworkError(pState, "socket send failed");
                    continue;
                }
            } else if (Result > 0) {
                pState->pSendBuf     += Result;
                pState->SendNumBytes -= Result;
                _Terminal_AddBytesSent(pState, (unsigned)Result);
            }
        }

        SYS_Sleep(1);
        RTTBridge_IncrementPolls();
    }

    Log_Print("Terminal service thread exiting\n");
}


/*************************** End of file ****************************/
