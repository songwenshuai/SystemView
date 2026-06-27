/*********************************************************************
*                             CineLogic                              *
*                      RTT Trace and Debug Bridge                    *
**********************************************************************
----------------------------------------------------------------------
File    : SystemViewHandshake.c
Purpose : SystemView TCP handshake handling
---------------------------END-OF-HEADER------------------------------
*/

#include <string.h>

#include "SystemView_internal.h"
#include "Log.h"

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
bool _SystemView_PerformHandshake(SystemView_State_t *pState, SYS_SOCKET_HANDLE hSock) {
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


/*************************** End of file ****************************/
