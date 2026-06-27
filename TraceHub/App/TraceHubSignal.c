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
File    : TraceHubSignal.c
Purpose : TraceHub process signal handling
---------------------------END-OF-HEADER------------------------------
*/

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include <string.h>

#if defined(_WIN32)
  #include <io.h>
  #include <process.h>
  #ifndef STDERR_FILENO
    #define STDERR_FILENO 2
  #endif
  #define TRACEHUB_SIGNAL_WRITE _write
  #define TRACEHUB_SIGNAL_EXIT  _exit
#else
  #include <unistd.h>
  #define TRACEHUB_SIGNAL_WRITE write
  #define TRACEHUB_SIGNAL_EXIT  _exit
#endif

#include "TraceHubSignal.h"
#include "Log.h"

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

static volatile sig_atomic_t _running = 1;

/*********************************************************************
*
*       Static functions
*
**********************************************************************
*/

/*********************************************************************
*
*       _SignalHandler()
*
*  Function description
*    Signal handler for graceful shutdown.
*/
static void _SignalHandler(int signum) {
    _running = 0;

    if (signum == SIGILL || signum == SIGFPE ||
        signum == SIGSEGV || signum == SIGABRT) {
        static const char message[] = "tracehub: fatal signal received\n";

        (void)TRACEHUB_SIGNAL_WRITE(STDERR_FILENO, message, sizeof(message) - 1u);
        TRACEHUB_SIGNAL_EXIT(128 + signum);
    }
}

/*********************************************************************
*
*       Public functions
*
**********************************************************************
*/

/*********************************************************************
*
*       TraceHubSignal_Init()
*
*  Function description
*    Initialize signal handlers for graceful shutdown.
*/
void TraceHubSignal_Init(void) {
    _running = 1;

#if defined(_WIN32)
    signal(SIGINT, _SignalHandler);
    signal(SIGTERM, _SignalHandler);
    signal(SIGILL, _SignalHandler);
    signal(SIGFPE, _SignalHandler);
    signal(SIGSEGV, _SignalHandler);
    signal(SIGABRT, _SignalHandler);
#else
    struct sigaction action;

    memset(&action, 0, sizeof(action));
    action.sa_handler = _SignalHandler;
    sigemptyset(&action.sa_mask);

    if (sigaction(SIGINT, &action, NULL) != 0) {
        Log_Error("Failed to install SIGINT handler\n");
    }
    if (sigaction(SIGTERM, &action, NULL) != 0) {
        Log_Error("Failed to install SIGTERM handler\n");
    }
    if (sigaction(SIGILL, &action, NULL) != 0) {
        Log_Error("Failed to install SIGILL handler\n");
    }
    if (sigaction(SIGFPE, &action, NULL) != 0) {
        Log_Error("Failed to install SIGFPE handler\n");
    }
    if (sigaction(SIGSEGV, &action, NULL) != 0) {
        Log_Error("Failed to install SIGSEGV handler\n");
    }
    if (sigaction(SIGABRT, &action, NULL) != 0) {
        Log_Error("Failed to install SIGABRT handler\n");
    }
#endif
}

/*********************************************************************
*
*       TraceHubSignal_GetRunFlag()
*
*  Function description
*    Return the process run flag shared by RTT bridge and run loop.
*/
volatile sig_atomic_t *TraceHubSignal_GetRunFlag(void) {
    return &_running;
}

/*************************** End of file ****************************/
