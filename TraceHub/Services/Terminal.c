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
*                                                                    *
*       CineLogic TraceHub * RTT trace and debug bridge              *
*                                                                    *
**********************************************************************
*                                                                    *
* All rights reserved.                                               *
*                                                                    *
* CineLogic strongly recommends to not make any changes              *
* to or modify the source code of this software in order to stay     *
* compatible with the SharedMem and RTT data path.                   *
*                                                                    *
* Redistribution and use in source and binary forms, with or         *
* without modification, are permitted provided that the following    *
* condition is met:                                                  *
*                                                                    *
* o Redistributions of source code must retain the above copyright   *
*   notice, this condition and the following disclaimer.             *
*                                                                    *
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND             *
* CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,        *
* INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF           *
* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE           *
* DISCLAIMED. IN NO EVENT SHALL CINELOGIC BE LIABLE FOR              *
* ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR           *
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT  *
* OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;    *
* OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF      *
* LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT          *
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE  *
* USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH   *
* DAMAGE.                                                            *
*                                                                    *
**********************************************************************
----------------------------------------------------------------------
File    : Terminal.c
Purpose : Terminal service for RTT Channel 0 with Telnet protocol
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

#if defined(_WIN32)
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
  #endif
  #include <windows.h>
  #include <conio.h>
  #include <io.h>
  typedef SSIZE_T ssize_t;
  #ifndef STDIN_FILENO
    #define STDIN_FILENO 0
  #endif
  #ifndef STDOUT_FILENO
    #define STDOUT_FILENO 1
  #endif
  #define read  _read
  #define write _write
#else
  #include <termios.h>
  #include <unistd.h>
  #include <sys/select.h>
#endif

#include "Terminal.h"
#include "CoreLogRecorder.h"
#include "RTTBridge.h"
#include "Socket.h"
#include "SYS.h"
#include "Log.h"
#include "TelnetCodec.h"
#include "ByteQueue.h"

/*********************************************************************
*
*       Defines, fixed
*
**********************************************************************
*/

/*********************************************************************
*
*       TERMINAL_SEND_BUF_SIZE
*  Size of send buffer for RTT to socket data transfer.
*
*/
#define TERMINAL_SEND_BUF_SIZE      8192

/*********************************************************************
*
*       TERMINAL_RECV_BUF_SIZE
*  Size of receive buffer for socket to RTT data transfer.
*
*/
#define TERMINAL_RECV_BUF_SIZE      8192

/*********************************************************************
*
*       TERMINAL_DEFAULT_NETWORK_QUEUE_SIZE
*  Size of queued RTT output retained for TCP clients.
*
*/
#ifndef TERMINAL_DEFAULT_NETWORK_QUEUE_SIZE
  #define TERMINAL_DEFAULT_NETWORK_QUEUE_SIZE (1024u * 1024u)
#endif

/*********************************************************************
*
*       TERMINAL_RECOVERY_DELAY_MS
*  Delay between recoverable RTT access retries.
*
*/
#define TERMINAL_RECOVERY_DELAY_MS      100

/*********************************************************************
*
*       TERMINAL_IDLE_DELAY_MS
*  Maximum delay after which available data from RTT buffer is sent.
*
*/
#define TERMINAL_IDLE_DELAY_MS      20

/*********************************************************************
*
*       Types
*
**********************************************************************
*/

/*********************************************************************
*
*       Terminal_State_t
*
*  Description
*    Runtime state of the Terminal service.
*/
typedef struct {
    // Configuration
    Terminal_Config_t   Config;

    // Runtime state
    bool                Initialized;
    bool                Running;
    bool                FatalError;
    bool                LockInitialized;
    SYS_SOCKET_HANDLE   hListener;
    SYS_SOCKET_HANDLE   hClient;
    SYS_Thread          ServiceThread;
    SYS_Thread          DrainThread;
    bool                ServiceThreadStarted;
    bool                DrainThreadStarted;
    bool                CoreConsumerRegistered;
    bool                ClientDisconnectRequested;
    bool                TargetInputEnabled;
    bool                RTTRecovering;
    SYS_Mutex           Lock;

    // Transmission state management for partial send/receive
    int                 SendNumBytes;
    int                 WriteNumBytes;
    char               *pSendBuf;
    char               *pWriteBuf;
    char                acSendBuf[TERMINAL_SEND_BUF_SIZE];
    char                acWriteBuf[TERMINAL_RECV_BUF_SIZE];
    char               *pNetworkQueue;
    ByteQueue_t         NetworkQueue;

    TelnetCodec_State_t     TelnetState;

    // Statistics
    unsigned            BytesSent;
    unsigned            BytesReceived;
    unsigned            ConnectionsCount;
} Terminal_State_t;

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

static Terminal_State_t _terminal_state;

/*********************************************************************
*
*       Console mode static data
*
**********************************************************************
*/

#if defined(_WIN32)
static HANDLE _console_input = NULL;
static DWORD  _orig_console_mode = 0;
#else
static struct termios _orig_termios;
#endif
static bool   _termios_saved = false;

/*********************************************************************
*
*       Static functions
*
**********************************************************************
*/

/*********************************************************************
*
*       _Terminal_GetConfiguredQueueSize()
*
*  Function description
*    Return the configured network queue capacity.
*/
static size_t _Terminal_GetNetworkQueueSize(void) {
    return TERMINAL_DEFAULT_NETWORK_QUEUE_SIZE;
}

/*********************************************************************
*
*       _Console_SetRawMode()
*
*  Function description
*    Configure terminal for raw mode (character-at-a-time input).
*    Disables canonical mode, echo, and signal generation.
*
*  Return value
*    0   Success
*   -1   Failed to configure terminal
*/
static int _Console_SetRawMode(void) {
#if defined(_WIN32)
    DWORD raw_mode;

    _console_input = GetStdHandle(STD_INPUT_HANDLE);
    if (_console_input == INVALID_HANDLE_VALUE || _console_input == NULL) {
        Log_Error("Console: failed to get console input handle\n");
        return -1;
    }
    if (!GetConsoleMode(_console_input, &_orig_console_mode)) {
        Log_Error("Console: failed to get console mode: %lu\n", GetLastError());
        return -1;
    }
    _termios_saved = true;

    raw_mode = _orig_console_mode;
    raw_mode &= ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT);
    raw_mode |= ENABLE_PROCESSED_INPUT;
    if (!SetConsoleMode(_console_input, raw_mode)) {
        Log_Error("Console: failed to set raw mode: %lu\n", GetLastError());
        return -1;
    }

    Log_Print("Console: terminal set to raw mode\n");
    return 0;
#else
    struct termios raw;
    //
    // Save original terminal settings
    //
    if (tcgetattr(STDIN_FILENO, &_orig_termios) < 0) {
        Log_Error("Console: failed to get terminal attributes: %s\n", strerror(errno));
        return -1;
    }
    _termios_saved = true;
    //
    // Configure raw mode
    // Keep ISIG enabled so Ctrl+C generates SIGINT for graceful shutdown
    //
    raw = _orig_termios;
    raw.c_lflag &= ~(ICANON | ECHO | IEXTEN);  // Disable canonical, echo, extended input
    raw.c_iflag &= ~(IXON | ICRNL | BRKINT | INPCK | ISTRIP);  // Disable XON/XOFF, CR->NL
    raw.c_oflag &= ~(OPOST);  // Disable output processing
    raw.c_cflag |= CS8;       // 8-bit characters
    raw.c_cc[VMIN]  = 0;      // Non-blocking read
    raw.c_cc[VTIME] = 0;      // No timeout

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) < 0) {
        Log_Error("Console: failed to set raw mode: %s\n", strerror(errno));
        return -1;
    }

    Log_Print("Console: terminal set to raw mode\n");
    return 0;
#endif
}

/*********************************************************************
*
*       _Console_RestoreMode()
*
*  Function description
*    Restore terminal to original settings.
*/
static void _Console_RestoreMode(void) {
    if (_termios_saved) {
#if defined(_WIN32)
        SetConsoleMode(_console_input, _orig_console_mode);
#else
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &_orig_termios);
#endif
        _termios_saved = false;
        Log_Print("Console: terminal restored to original mode\n");
    }
}

/*********************************************************************
*
*       _Console_CheckInput()
*
*  Function description
*    Check if there is input available on stdin (non-blocking).
*
*  Return value
*    1   Input available
*    0   No input available
*   -1   Error
*/
static int _Console_CheckInput(void) {
#if defined(_WIN32)
    return _kbhit() ? 1 : 0;
#else
    struct timeval tv = {0, 0};
    fd_set         fds;

    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);

    return select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv);
#endif
}

/*********************************************************************
*
*       _Terminal_IsRunning()
*
*  Function description
*    Read the service running state through the module lock.
*/
static bool _Terminal_IsRunning(Terminal_State_t *pState) {
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
static void _Terminal_SetRunning(Terminal_State_t *pState, bool running) {
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
static bool _Terminal_HasFatalError(Terminal_State_t *pState) {
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
static void _Terminal_ReportRTTError(Terminal_State_t *pState, const char *operation) {
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
static SYS_SOCKET_HANDLE _Terminal_GetClient(Terminal_State_t *pState) {
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
static void _Terminal_ResetConnectionStateLocked(Terminal_State_t *pState) {
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
static void _Terminal_ClearNetworkQueueLocked(Terminal_State_t *pState) {
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
static void _Terminal_RequestNetworkStreamResetLocked(Terminal_State_t *pState) {
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
static void _Terminal_ReportRecoverableRTTError(Terminal_State_t *pState, const char *operation) {
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
static void _Terminal_ReportRTTRecovered(Terminal_State_t *pState) {
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
static unsigned _Terminal_SetConnectedClient(Terminal_State_t *pState, SYS_SOCKET_HANDLE hClient) {
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
static void _Terminal_AddBytesSent(Terminal_State_t *pState, unsigned NumBytes) {
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
static void _Terminal_AddBytesReceived(Terminal_State_t *pState, unsigned NumBytes) {
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
static int _Terminal_QueueTargetData(Terminal_State_t *pState, const char *data, unsigned num_bytes) {
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
static unsigned _Terminal_DequeueTargetData(Terminal_State_t *pState, char *buffer, unsigned buffer_size) {
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
static int _Terminal_RegisterCoreConsumer(Terminal_State_t *pState) {
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
static void _Terminal_UnregisterCoreConsumer(Terminal_State_t *pState) {
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
static int _Terminal_CheckChannels(Terminal_State_t *pState) {
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
static int _Terminal_ReadTargetData(Terminal_State_t *pState, char *buffer, size_t buffer_size) {
    if ((pState == NULL) || (buffer == NULL) || (buffer_size == 0u)) {
        return -1;
    }

    if (pState->CoreConsumerRegistered) {
        return CoreLogRecorder_ReadChannel(pState->Config.channel, buffer, buffer_size);
    }

    return RTTBridge_ReadUpBufferNoLock(pState->Config.channel, buffer, buffer_size);
}

/*********************************************************************
*
*       _Terminal_ConsoleThread()
*
*  Function description
*    Console service thread for direct stdin/stdout RTT communication.
*    Handles character-at-a-time terminal interaction.
*
*  Parameters
*    pArg - Unused parameter (required by thread API)
*/
static void _Terminal_ConsoleThread(void *pArg) {
    (void)pArg;

    Terminal_State_t *pState = &_terminal_state;
    uintptr_t         Address;
    int               NumBytes;
    unsigned char     InputChar;
    int               Result;

    Log_Print("Console service thread started on channel %u\n", pState->Config.channel);

    while (_Terminal_IsRunning(pState)) {
        if (RTTBridge_GetValidatedRTTRegion(&Address, NULL) != 0) {
            _Terminal_ReportRecoverableRTTError(pState, "region validation");
            SYS_Sleep(TERMINAL_RECOVERY_DELAY_MS);
            continue;
        }

        //
        // Check for input from stdin
        // Note: ISIG is enabled, Ctrl+C will generate SIGINT automatically
        //
        if (pState->TargetInputEnabled) {
            Result = _Console_CheckInput();
            if (Result > 0) {
                ssize_t n = read(STDIN_FILENO, &InputChar, 1);
                if (n == 1) {
                    //
                    // Write character to RTT down buffer
                    //
                    if (pState->WriteNumBytes == 0) {
                        pState->acWriteBuf[0]  = (char)InputChar;
                        pState->WriteNumBytes  = 1;
                        pState->pWriteBuf      = pState->acWriteBuf;
                        _Terminal_AddBytesReceived(pState, 1u);
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
        // Read new target output only if previous data has been fully sent
        //
        if (pState->SendNumBytes == 0) {
            NumBytes = _Terminal_ReadTargetData(pState,
                                                pState->acSendBuf,
                                                sizeof(pState->acSendBuf));
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
                pState->SendNumBytes = NumBytes;
                pState->pSendBuf     = pState->acSendBuf;
            }
        }

        //
        // Output pending data to stdout
        //
        if (pState->SendNumBytes > 0) {
            ssize_t written = write(STDOUT_FILENO, pState->pSendBuf, pState->SendNumBytes);
            if (written > 0) {
                pState->pSendBuf     += written;
                pState->SendNumBytes -= (int)written;
                _Terminal_AddBytesSent(pState, (unsigned)written);
            }
        }

        SYS_Sleep(1);
        RTTBridge_IncrementPolls();
    }

    Log_Print("Console service thread exiting\n");
}

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
static void _Terminal_DrainThread(void *pArg) {
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
static void _Terminal_CloseClient(Terminal_State_t *pState) {
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
static bool _Terminal_TakeClientDisconnectRequest(Terminal_State_t *pState) {
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
static void _Terminal_CloseClientForNetworkError(Terminal_State_t *pState,
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
static void _Terminal_ServiceThread(void *pArg) {
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

/*********************************************************************
*
*       Public functions
*
**********************************************************************
*/

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
