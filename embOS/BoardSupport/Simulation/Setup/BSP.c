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
File    : BSP.c
Purpose : BSP for embOS simulation.
*/

#if !defined(EMBOS_SIM_HOST_WINDOWS) || !defined(EMBOS_SIM_HOST_POSIX) || !defined(EMBOS_SIM_GUI)
  #error "EMBOS_SIM_HOST_WINDOWS, EMBOS_SIM_HOST_POSIX and EMBOS_SIM_GUI must be defined by the simulation build."
#endif

#if ((EMBOS_SIM_HOST_WINDOWS != 0) && (EMBOS_SIM_HOST_POSIX != 0))
  #error "Only one simulation host platform can be enabled."
#endif

#if ((EMBOS_SIM_HOST_WINDOWS == 0) && (EMBOS_SIM_HOST_POSIX == 0))
  #error "Unsupported simulation platform."
#endif

#include "BSP.h"
#if (EMBOS_SIM_GUI != 0)
#include "SIM_OS.h"
#endif
#if (EMBOS_SIM_HOST_POSIX != 0)
#include <pthread.h>
#include <stdlib.h>
#endif
#include <stdio.h>
#include <string.h>

/*********************************************************************
*
*       Configuration
*
**********************************************************************
*/

//
// If BSP_ENABLE_PRINT_LEDS is 1 the state of the LEDs is printed to stdout after an LED is set, cleared or toggled.
//
#ifndef   BSP_ENABLE_PRINT_LEDS
  #if (EMBOS_SIM_GUI != 0)
    #define BSP_ENABLE_PRINT_LEDS  (0)
  #else
    #define BSP_ENABLE_PRINT_LEDS  (1)
  #endif
#endif

//
// Specifies the number of LEDs tracked by the BSP.
//
#ifndef   BSP_MAX_LEDS
  #if (EMBOS_SIM_HOST_WINDOWS != 0)
    #define BSP_MAX_LEDS  (32)
  #elif (EMBOS_SIM_GUI != 0)
    #define BSP_MAX_LEDS  (8)
  #else
    #define BSP_MAX_LEDS  (4)
  #endif
#endif

#if (BSP_MAX_LEDS < 1)
  #error "BSP_MAX_LEDS must be greater than zero."
#endif

#if (BSP_MAX_LEDS > 32)
  #error "BSP_MAX_LEDS must be between 1 and 32."
#endif

//
// Specifies the number of LEDs shown in the terminal output.
//
#ifndef   BSP_PRINT_LED_COUNT
  #if (EMBOS_SIM_HOST_POSIX != 0)
    #define BSP_PRINT_LED_COUNT  (BSP_MAX_LEDS)
  #elif (BSP_MAX_LEDS < 8)
    #define BSP_PRINT_LED_COUNT  (BSP_MAX_LEDS)
  #else
    #define BSP_PRINT_LED_COUNT  (8)
  #endif
#endif

#if ((BSP_PRINT_LED_COUNT < 1) || (BSP_PRINT_LED_COUNT > BSP_MAX_LEDS))
  #error "BSP_PRINT_LED_COUNT must be between 1 and BSP_MAX_LEDS."
#endif

//
// Selects whether terminal LED output rewrites the previous LED line.
//
#ifndef   BSP_PRINT_LEDS_REWRITE_LINE
  #if (EMBOS_SIM_HOST_POSIX != 0)
    #define BSP_PRINT_LEDS_REWRITE_LINE  (1)
  #else
    #define BSP_PRINT_LEDS_REWRITE_LINE  (0)
  #endif
#endif

//
// Selects whether BSP_Init() prints the initial LED state.
//
#ifndef   BSP_PRINT_LEDS_ON_INIT
  #if (EMBOS_SIM_HOST_WINDOWS != 0)
    #define BSP_PRINT_LEDS_ON_INIT  (1)
  #else
    #define BSP_PRINT_LEDS_ON_INIT  (0)
  #endif
#endif

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

#if (EMBOS_SIM_HOST_POSIX != 0)
static pthread_mutex_t _Lock = PTHREAD_MUTEX_INITIALIZER;
#endif
static unsigned char   _LEDState[BSP_MAX_LEDS];
#if (BSP_ENABLE_PRINT_LEDS != 0)
static char     _Symbol[2] = {' ', 'x'};
#if (BSP_PRINT_LEDS_REWRITE_LINE != 0)
static int      _LEDLineActive;
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
*       _FailPthread()
*/
#if (EMBOS_SIM_HOST_POSIX != 0)
static void _FailPthread(const char* sText, int Error) {
  fprintf(stderr, "[ERROR] %s failed: %d\n", sText, Error);
  exit(EXIT_FAILURE);
}
#endif

/*********************************************************************
*
*       _LockLEDState()
*/
static void _LockLEDState(void) {
#if (EMBOS_SIM_HOST_POSIX != 0)
  int Error;

  Error = pthread_mutex_lock(&_Lock);
  if (Error != 0) {
    _FailPthread("pthread_mutex_lock", Error);
  }
#endif
}

/*********************************************************************
*
*       _UnlockLEDState()
*/
static void _UnlockLEDState(void) {
#if (EMBOS_SIM_HOST_POSIX != 0)
  int Error;

  Error = pthread_mutex_unlock(&_Lock);
  if (Error != 0) {
    _FailPthread("pthread_mutex_unlock", Error);
  }
#endif
}

/*********************************************************************
*
*       _IsLEDIndexValid()
*/
static int _IsLEDIndexValid(int Index) {
  return ((Index >= 0) && (Index < BSP_MAX_LEDS));
}

/*********************************************************************
*
*       _UpdateWindow()
*/
static void _UpdateWindow(void) {
#if (EMBOS_SIM_GUI != 0)
  SIM_OS_UpdateWindow();
#endif
}

/*********************************************************************
*
*       _PrintLEDState()
*/
#if (BSP_ENABLE_PRINT_LEDS != 0)
static void _PrintLEDState(void) {
  int i;

#if (BSP_PRINT_LEDS_REWRITE_LINE != 0)
  if (_LEDLineActive == 0) {
    printf("\033[H\033[J");
  } else {
    printf("\033[F\033[K");
  }
#endif
  for (i = 0; i < BSP_PRINT_LED_COUNT; ++i) {
    printf("[%c] ", _Symbol[(int)_LEDState[i]]);
  }
  printf("\n");
  fflush(stdout);
#if (BSP_PRINT_LEDS_REWRITE_LINE != 0)
  _LEDLineActive = 1;
#endif
}
#endif

/*********************************************************************
*
*       _PrintLEDStateOnChange()
*/
static void _PrintLEDStateOnChange(void) {
#if (BSP_ENABLE_PRINT_LEDS != 0)
  _PrintLEDState();
#endif
}

/*********************************************************************
*
*       _SetLEDState()
*/
static void _SetLEDState(int Index, unsigned char State) {
  if (_IsLEDIndexValid(Index) == 0) {
    return;
  }
  _LockLEDState();
  _LEDState[Index] = State;
  _PrintLEDStateOnChange();
  _UnlockLEDState();
  _UpdateWindow();
}

/*********************************************************************
*
*       _ToggleLEDState()
*/
static void _ToggleLEDState(int Index) {
  if (_IsLEDIndexValid(Index) == 0) {
    return;
  }
  _LockLEDState();
  _LEDState[Index] ^= 1u;
  _PrintLEDStateOnChange();
  _UnlockLEDState();
  _UpdateWindow();
}

/*********************************************************************
*
*       Global functions
*
**********************************************************************
*/

/*********************************************************************
*
*       BSP_Init()
*/
void BSP_Init(void) {
  _LockLEDState();
  memset(_LEDState, 0, sizeof(_LEDState));
#if ((BSP_ENABLE_PRINT_LEDS != 0) && (BSP_PRINT_LEDS_REWRITE_LINE != 0))
  _LEDLineActive = 0;
#endif
#if ((BSP_ENABLE_PRINT_LEDS != 0) && (BSP_PRINT_LEDS_ON_INIT != 0))
  _PrintLEDState();
#endif
  _UnlockLEDState();
  _UpdateWindow();
}

/*********************************************************************
*
*       BSP_SetLED()
*/
void BSP_SetLED(int Index) {
  _SetLEDState(Index, 1u);
}

/*********************************************************************
*
*       BSP_ClrLED()
*/
void BSP_ClrLED(int Index) {
  _SetLEDState(Index, 0u);
}

/*********************************************************************
*
*       BSP_ToggleLED()
*/
void BSP_ToggleLED(int Index) {
  _ToggleLEDState(Index);
}

/*********************************************************************
*
*       BSP_GetLEDState()
*/
int BSP_GetLEDState(int Index) {
  int State;

  if (_IsLEDIndexValid(Index) == 0) {
    return 0;
  }
  _LockLEDState();
  State = (_LEDState[Index] != 0u) ? (int)(1u << Index) : 0;
  _UnlockLEDState();
  return State;
}

/*************************** End of file ****************************/
