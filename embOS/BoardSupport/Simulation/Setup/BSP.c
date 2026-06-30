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

#if (EMBOS_SIM_HOST_WINDOWS != 0)

#include "BSP.h"
#if (EMBOS_SIM_GUI != 0)
#include "SIM_OS.h"
#endif
#include <stdio.h>

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
// Specifies the number of LEDs shown in the terminal output.
//
#ifndef   BSP_PRINT_LED_COUNT
  #define BSP_PRINT_LED_COUNT  (8)
#endif

#if ((BSP_PRINT_LED_COUNT < 1) || (BSP_PRINT_LED_COUNT > 32))
  #error "BSP_PRINT_LED_COUNT must be between 1 and 32."
#endif

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

static unsigned int LEDs;
#if (BSP_ENABLE_PRINT_LEDS != 0)
static char     _Symbol[2] = {' ', 'x'};
#endif

/*********************************************************************
*
*       Local functions
*
**********************************************************************
*/

/*********************************************************************
*
*       _IsLEDIndexValid()
*/
static int _IsLEDIndexValid(int Index) {
  return ((Index >= 0) && (Index < 32));
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

  for (i = 0; i < BSP_PRINT_LED_COUNT; ++i) {
    printf("[%c] ", _Symbol[(BSP_GetLEDState(i) != 0)]);
  }
  printf("\n");
  fflush(stdout);
}
#endif

/*********************************************************************
*
*       _UpdateOutput()
*/
static void _UpdateOutput(void) {
#if (BSP_ENABLE_PRINT_LEDS != 0)
  _PrintLEDState();
#endif
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
  LEDs = 0;
  _UpdateOutput();
}

/*********************************************************************
*
*       BSP_SetLED()
*/
void BSP_SetLED(int Index) {
  if (_IsLEDIndexValid(Index) != 0) {
    LEDs |= (1u << Index);
    _UpdateOutput();
  }
}

/*********************************************************************
*
*       BSP_ClrLED()
*/
void BSP_ClrLED(int Index) {
  if (_IsLEDIndexValid(Index) != 0) {
    LEDs &= ~(1u << Index);
    _UpdateOutput();
  }
}

/*********************************************************************
*
*       BSP_ToggleLED()
*/
void BSP_ToggleLED(int Index) {
  if (_IsLEDIndexValid(Index) != 0) {
    LEDs ^= (1u << Index);
    _UpdateOutput();
  }
}

/*********************************************************************
*
*       BSP_GetLEDState()
*/
int BSP_GetLEDState(int Index) {
  if (_IsLEDIndexValid(Index) == 0) {
    return 0;
  }
  return (LEDs & (1u << Index));
}

#elif (EMBOS_SIM_HOST_POSIX != 0)

#include "BSP.h"
#if (EMBOS_SIM_GUI != 0)
#include "SIM_OS.h"
#endif
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
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
// Specifies the number of LEDs.
//
#ifndef   BSP_MAX_LEDS
  #if (EMBOS_SIM_GUI != 0)
    #define BSP_MAX_LEDS  (8)
  #else
    #define BSP_MAX_LEDS  (4)
  #endif
#endif

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

static pthread_mutex_t _Lock = PTHREAD_MUTEX_INITIALIZER;
static char            _LEDState[BSP_MAX_LEDS];
#if (BSP_ENABLE_PRINT_LEDS != 0)
static char     _Symbol[2] = {' ', 'x'};
static int      _LEDLineActive;
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
static void _FailPthread(const char* sText, int Error) {
  fprintf(stderr, "[ERROR] %s failed: %d\n", sText, Error);
  exit(EXIT_FAILURE);
}

/*********************************************************************
*
*       _LockLEDState()
*/
static void _LockLEDState(void) {
  int Error;

  Error = pthread_mutex_lock(&_Lock);
  if (Error != 0) {
    _FailPthread("pthread_mutex_lock", Error);
  }
}

/*********************************************************************
*
*       _UnlockLEDState()
*/
static void _UnlockLEDState(void) {
  int Error;

  Error = pthread_mutex_unlock(&_Lock);
  if (Error != 0) {
    _FailPthread("pthread_mutex_unlock", Error);
  }
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

  if (_LEDLineActive == 0) {
    printf("\033[H\033[J");
  } else {
    printf("\033[F\033[K");
  }
  for (i = 0; i < BSP_MAX_LEDS; ++i) {
    printf("[%c] ", _Symbol[(int)_LEDState[i]]);
  }
  printf("\n");
  fflush(stdout);
  _LEDLineActive = 1;
}
#endif

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
#if (BSP_ENABLE_PRINT_LEDS != 0)
  _LEDLineActive = 0;
#endif
  _UnlockLEDState();
  _UpdateWindow();
}

/*********************************************************************
*
*       BSP_SetLED()
*/
void BSP_SetLED(int Index) {
  if (_IsLEDIndexValid(Index) == 0) {
    return;
  }
  _LockLEDState();
  _LEDState[Index] = 1;
#if (BSP_ENABLE_PRINT_LEDS != 0)
  _PrintLEDState();
#endif
  _UnlockLEDState();
  _UpdateWindow();
}

/*********************************************************************
*
*       BSP_ClrLED()
*/
void BSP_ClrLED(int Index) {
  if (_IsLEDIndexValid(Index) == 0) {
    return;
  }
  _LockLEDState();
  _LEDState[Index] = 0;
#if (BSP_ENABLE_PRINT_LEDS != 0)
  _PrintLEDState();
#endif
  _UnlockLEDState();
  _UpdateWindow();
}

/*********************************************************************
*
*       BSP_ToggleLED()
*/
void BSP_ToggleLED(int Index) {
  if (_IsLEDIndexValid(Index) == 0) {
    return;
  }
  _LockLEDState();
  _LEDState[Index] ^= 1;
#if (BSP_ENABLE_PRINT_LEDS != 0)
  _PrintLEDState();
#endif
  _UnlockLEDState();
  _UpdateWindow();
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
  State = _LEDState[Index];
  _UnlockLEDState();
  return State;
}

#else
  #error "Unsupported simulation platform."
#endif

/*************************** End of file ****************************/
