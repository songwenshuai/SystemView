/*********************************************************************
*                     SEGGER Microcontroller GmbH                    *
*                        The Embedded Experts                        *
**********************************************************************
*                                                                    *
*       (c) 1995 - 2026 SEGGER Microcontroller GmbH                  *
*                                                                    *
*       Internet: segger.com  Support: support_embos@segger.com      *
*                                                                    *
**********************************************************************
*                                                                    *
*       embOS-Classic * Real time operating system                   *
*                                                                    *
*       Please note:                                                 *
*                                                                    *
*       Knowledge of this file may under no circumstances            *
*       be used to write a similar product or a real-time            *
*       operating system for in-house use.                           *
*                                                                    *
*       Thank you for your fairness !                                *
*                                                                    *
**********************************************************************
*                                                                    *
*       OS version: V5.22.0.0                                        *
*                                                                    *
**********************************************************************

-------------------------- END-OF-HEADER -----------------------------
File    : PosixMain.c
Purpose : POSIX entry point used to initialize the simulation before
          calling the application entry point.
*/

#include <SDL.h>
#include <stdio.h>
#include "SIM_OS.h"

/*********************************************************************
*
*       Types, local
*
**********************************************************************
*/

typedef struct {
  SDL_atomic_t Done;
  int          Result;
} APP_CONTEXT;

/*********************************************************************
*
*       Prototypes
*
**********************************************************************
*/

#if defined(__cplusplus) && (!defined(SIM_OS_APP_MAIN_CXX) || (SIM_OS_APP_MAIN_CXX == 0))
extern "C" {
#endif
int SIM_OS_AppMain(void);
#if defined(__cplusplus) && (!defined(SIM_OS_APP_MAIN_CXX) || (SIM_OS_APP_MAIN_CXX == 0))
}
#endif

#if defined(EMBOS_SIM_ENABLE_ASAN) && (EMBOS_SIM_ENABLE_ASAN != 0)
#if defined(__cplusplus)
extern "C" {
#endif
int __lsan_do_recoverable_leak_check(void);
#if defined(__cplusplus)
}
#endif
#endif

/*********************************************************************
*
*       Local functions
*
**********************************************************************
*/

/*********************************************************************
*
*       _AppThread()
*
*  Function description
*    Application main thread.
*
*  Parameters
*    pData: A pointer to the application context.
*
*  Return value
*    Return value of the application entry point.
*/
static int _AppThread(void* pData) {
  APP_CONTEXT* pContext;

  pContext         = (APP_CONTEXT*)pData;
  pContext->Result = SIM_OS_AppMain();
  SDL_AtomicSet(&pContext->Done, 1);
  SIM_OS_RequestQuit();
  return pContext->Result;
}

/*********************************************************************
*
*       _CheckLeaks()
*
*  Function description
*    Runs LeakSanitizer before SDL unloads host GUI driver modules.
*
*  Return value
*    == 0: No leaks detected or LeakSanitizer is not enabled.
*    != 0: LeakSanitizer reported leaks.
*/
static int _CheckLeaks(void) {
#if defined(EMBOS_SIM_ENABLE_ASAN) && (EMBOS_SIM_ENABLE_ASAN != 0)
  return __lsan_do_recoverable_leak_check();
#else
  return 0;
#endif
}

/*********************************************************************
*
*       Global functions
*
**********************************************************************
*/

/*********************************************************************
*
*       main()
*
*  Function description
*    Keeps the SDL video event loop on the process main thread and runs
*    the embOS application on a separate host thread.
*
*  Parameters
*    argc: The number of command line arguments.
*    argv: A pointer to the command line arguments.
*
*  Return value
*    Return value of the application entry point, or 1 if the application
*    thread could not be created.
*/
int main(int argc, char** argv) {
  APP_CONTEXT Context;
  SDL_Thread* pThread;
  int         ThreadResult;
  int         WindowResult;

  //
  // Avoid warnings for unused parameters.
  //
  (void)argc;
  (void)argv;
  //
  // Initialize the POSIX specific components of the embOS simulation.
  //
  SDL_SetMainReady();
  SDL_AtomicSet(&Context.Done, 0);
  Context.Result = 0;
  SIM_OS_InitWindow();
  //
  // Call the main entry point of the application on a separate thread.
  //
  pThread = SDL_CreateThread(_AppThread, "embOS Application", &Context);
  if (pThread == NULL) {
    fprintf(stderr, "[ERROR] Could not create application thread: %s\n", SDL_GetError());
    SIM_OS_ShutdownWindow();
    return 1;
  }
  //
  // Handle the SDL video event loop on the process main thread.
  //
  WindowResult = SIM_OS_RunWindow();
  if (SDL_AtomicGet(&Context.Done) == 0) {
    OS_SIM_RequestStop();
  }
  SDL_WaitThread(pThread, &ThreadResult);
  Context.Result = ThreadResult;
  if ((WindowResult != 0) && (Context.Result == 0)) {
    Context.Result = WindowResult;
  }
  if ((_CheckLeaks() != 0) && (Context.Result == 0)) {
    Context.Result = 1;
  }
  SIM_OS_ShutdownWindow();
  return Context.Result;
}

/*************************** End of file ****************************/
