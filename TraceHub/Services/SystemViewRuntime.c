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
*       Static functions
*
**********************************************************************
*/

/*********************************************************************
*
*       _SystemView_ReceiveAppPacket()
*
*  Function description
*    Receive one length-prefixed SystemView App command packet.
*
*  Return value
*     0  No complete packet is ready yet.
*     1  A complete packet is ready in pState->pWriteBuf.
*    -1  Network error or disconnect.
*/
static int _SystemView_ReceiveAppPacket(SystemView_State_t *pState,
                                        SYS_SOCKET_HANDLE hClient) {
    unsigned char Len;
    unsigned      NumBytes;
    int           Result;

    if (pState->WriteNumBytes != 0) {
        return 0;
    }

    if (pState->AppPacketExpected == 0u) {
        Result = SYS_SOCKET_IsReadable(hClient, 0);
        if (Result < 0) {
            _SystemView_CloseClientForNetworkError(pState, "socket read wait failed");
            return -1;
        }
        if (Result == 0) {
            return 0;
        }

        Result = SYS_SOCKET_Receive(hClient, &Len, 1u);
        if (Result < 0) {
            if (Result != SYS_SOCKET_ERR_WOULDBLOCK) {
                _SystemView_CloseClientForNetworkError(pState, "socket receive failed");
                return -1;
            }
            return 0;
        }
        if (Result == 0) {
            _SystemView_CloseClientForNetworkDisconnect(pState,
                                                        "client disconnected before app packet length");
            return -1;
        }

        _SystemView_AddBytesReceived(pState, 1u);
        pState->AppPacketExpected = (unsigned)Len;
        pState->AppPacketReceived = 0u;
        if (pState->AppPacketExpected == 0u) {
            return 0;
        }
        if (pState->AppPacketExpected > SYSVIEW_APP_PACKET_MAX_SIZE) {
            _SystemView_CloseClientForNetworkError(pState, "invalid app packet length");
            return -1;
        }
    }

    while (pState->AppPacketReceived < pState->AppPacketExpected) {
        Result = SYS_SOCKET_IsReadable(hClient, 0);
        if (Result < 0) {
            _SystemView_CloseClientForNetworkError(pState, "socket read wait failed");
            return -1;
        }
        if (Result == 0) {
            return 0;
        }

        NumBytes = pState->AppPacketExpected - pState->AppPacketReceived;
        Result = SYS_SOCKET_Receive(hClient,
                                    pState->acWriteBuf + pState->AppPacketReceived,
                                    NumBytes);
        if (Result < 0) {
            if (Result != SYS_SOCKET_ERR_WOULDBLOCK) {
                _SystemView_CloseClientForNetworkError(pState, "socket receive failed");
                return -1;
            }
            return 0;
        }
        if (Result == 0) {
            _SystemView_CloseClientForNetworkDisconnect(pState,
                                                        "client disconnected during app packet payload");
            return -1;
        }

        pState->AppPacketReceived += (unsigned)Result;
        _SystemView_AddBytesReceived(pState, (unsigned)Result);
    }

    pState->WriteNumBytes = (int)pState->AppPacketExpected;
    pState->pWriteBuf = pState->acWriteBuf;
    pState->AppPacketExpected = 0u;
    pState->AppPacketReceived = 0u;
    return 1;
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
                SYS_SOCKET_Shutdown(hClient, SYS_SOCKET_SHUT_RDWR);
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
                SYS_SOCKET_Shutdown(hClient, SYS_SOCKET_SHUT_RDWR);
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

            Log_Info("SystemView: new client connected (total: %u)\n", ConnectionsCount);
        }

        if (_SystemView_ReceiveAppPacket(pState, hClient) < 0) {
            continue;
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
