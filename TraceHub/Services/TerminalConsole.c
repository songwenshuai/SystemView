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
File    : TerminalConsole.c
Purpose : Terminal console raw mode and console runtime
---------------------------END-OF-HEADER------------------------------
*/

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

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

#include "Terminal_internal.h"
#include "Log.h"
#include "RTTBridge.h"

#if defined(_WIN32)
static HANDLE _console_input = NULL;
static DWORD  _orig_console_mode = 0;
#else
static struct termios _orig_termios;
#endif
static bool   _termios_saved = false;

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
int _Console_SetRawMode(void) {
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
void _Console_RestoreMode(void) {
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
int _Console_CheckInput(void) {
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
*       _Terminal_ConsoleThread()
*
*  Function description
*    Console service thread for direct stdin/stdout RTT communication.
*    Handles character-at-a-time terminal interaction.
*
*  Parameters
*    pArg - Unused parameter (required by thread API)
*/
void _Terminal_ConsoleThread(void *pArg) {
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


/*************************** End of file ****************************/
