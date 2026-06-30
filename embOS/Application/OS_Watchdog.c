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
File    : OS_Watchdog.c
Purpose : embOS sample program for using the watchdog module
*/

#include "RTOS.h"

static OS_STACKPTR int StackHP[128], StackLP[128];  // Task stacks
static OS_TASK         TCBHP, TCBLP;                // Task control blocks
static OS_WD           WatchdogHP, WatchdogLP;
static OS_TICK_HOOK    Hook;

static void _TriggerWatchDog(void) {
  //
  // Trigger the hardware watchdog
  //
}

static void _Reset(OS_CONST_PTR OS_WD* pWD) {
  OS_USE_PARA(pWD);  // Applications can use pWD to detect WD expiration cause.
  //
  // Reboot microcontroller
  //
  while (1) {        // Dummy loop, you can set a breakpoint here
  }
}

static void HPTask(void) {
  OS_WD_Add(&WatchdogHP, 50);
  while (1) {
    OS_TASK_Delay(50);
    OS_WD_Trigger(&WatchdogHP);
  }
}

static void LPTask(void) {
  OS_WD_Add(&WatchdogLP, 200);
  while (1) {
    OS_TASK_Delay(200);
    OS_WD_Trigger(&WatchdogLP);
  }
}

/*********************************************************************
*
*       main()
*/
int main(void) {
  OS_Init();    // Initialize embOS
  OS_InitHW();  // Initialize required hardware
  OS_TASK_CREATE(&TCBHP, "HP Task", 100, HPTask, StackHP);
  OS_TASK_CREATE(&TCBLP, "LP Task",  50, LPTask, StackLP);
  OS_WD_Config(&_TriggerWatchDog, &_Reset);
  OS_TICK_AddHook(&Hook, OS_WD_Check);
  OS_Start();   // Start embOS
  return 0;
}

/*************************** End of file ****************************/
