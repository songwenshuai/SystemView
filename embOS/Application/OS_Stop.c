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
File    : OS_Stop.c
Purpose : embOS sample program for OS_Stop().
          This sample application stops embOS and returns from OS_Start()
*/

#include "RTOS.h"

#define BUFFER_SIZE    (256u)
static OS_U8           Buffer[BUFFER_SIZE];  // Buffer for main stack copy
static OS_MAIN_CONTEXT MainContext;          // Main context control structure
static OS_STACKPTR int StackHP[128];         // Task stack
static OS_TASK         TCBHP;                // Task control block

static void HPTask(void) {
  OS_TASK_Delay(50);
  OS_INT_Disable();
  OS_Stop();
}

/*********************************************************************
*
*       main()
*/
int main(void) {
  volatile int TheAnswerToEverything = 42;
  OS_Init();    // Initialize embOS
  OS_InitHW();  // Initialize required hardware
  OS_TASK_CREATE(&TCBHP, "HP Task", 100, HPTask, StackHP);
  OS_ConfigStop(&MainContext, Buffer, BUFFER_SIZE);
  OS_Start();   // Start embOS
  OS_DeInit();  // De-Initialize embOS
  //
  // We arrive here because OS_Stop() was called.
  // The local stack variable still has its value.
  //
  while (TheAnswerToEverything == 42) {
  }
  return 0;
}

/*************************** End of file ****************************/
