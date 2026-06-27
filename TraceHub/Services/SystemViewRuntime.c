/*********************************************************************
*                             CineLogic                              *
*                      RTT Trace and Debug Bridge                    *
**********************************************************************
----------------------------------------------------------------------
File    : SystemViewRuntime.c
Purpose : SystemView TCP service and RTT transfer runtime
---------------------------END-OF-HEADER------------------------------
*/

#include <stdint.h>
#include <stdio.h>

#include "SystemView_internal.h"
#include "Log.h"
#include "RTTBridge.h"

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
void _SystemView_ServiceThread(void *pArg) {
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
            Result = SYS_SOCKET_AcceptEx(pState->hListener,
                                         SYSVIEW_IDLE_DELAY_MS,
                                         &hClient);

            if (Result == SYS_SOCKET_ERR_ACCEPT_TIMEOUT) {
                continue;
            }

            if (Result != 0) {
                Log_Warn("SystemView: failed to accept connection\n");
                SYS_Sleep(1000);
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


/*************************** End of file ****************************/
