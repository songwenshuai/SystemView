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
File    : OS_SimBlocked.c
Purpose : embOS simulation sample application showing how to use
          blocking non-embOS functions from tasks.
*/

#if defined(WIN32) || defined(_WIN32)
  #define WIN32_LEAN_AND_MEAN
#endif

#include "RTOS.h"
#include "BSP.h"

#if defined(WIN32) || defined(_WIN32)
  #include <Windows.h>
#elif defined(__unix__) || defined(__APPLE__)
  #include <unistd.h>
#else
  #error "Unsupported simulation platform."
#endif

static OS_STACKPTR int StackHP[128], StackLP[128];  // Task stacks
static OS_TASK         TCBHP, TCBLP;                // Task control blocks

static void _SleepBlocked(void) {
#if defined(WIN32) || defined(_WIN32)
  Sleep(1000);
#elif defined(__unix__) || defined(__APPLE__)
  sleep(1);
#endif
}

static void HPTask(void) {
  while (1) {
    BSP_ToggleLED(0);
    OS_SIM_EnterSysCall();
    _SleepBlocked();
    OS_SIM_LeaveSysCall();
  }
}

static void LPTask(void) {
  while (1) {
    BSP_ToggleLED(1);
    OS_TASK_Delay(200);
  }
}

/*********************************************************************
*
*       main()
*/
int main(void) {
  OS_Init();    // Initialize embOS
  OS_InitHW();  // Initialize required hardware
  BSP_Init();   // Initialize LED ports
  OS_TASK_CREATE(&TCBHP, "HP Task", 100, HPTask, StackHP);
  OS_TASK_CREATE(&TCBLP, "LP Task",  50, LPTask, StackLP);
  OS_Start();   // Start embOS
  return 0;
}

/*************************** End of file ****************************/
