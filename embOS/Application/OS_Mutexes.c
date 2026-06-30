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
File    : OS_Mutexes.c
Purpose : embOS sample program demonstrating the usage of mutexes.
*/

#include "RTOS.h"

static OS_STACKPTR int StackHP[128], StackLP[128];  // Task stacks
static OS_TASK         TCBHP, TCBLP;                // Task control blocks
static OS_MUTEX        Mutex;

/*********************************************************************
*
*       _Write()
*/
static void _Write(char const* s) {
  OS_MUTEX_LockBlocked(&Mutex);
  OS_COM_SendString(s);
  OS_MUTEX_Unlock(&Mutex);
}

/*********************************************************************
*
*       HPTask()
*/
static void HPTask(void) {
  while (1) {
    _Write("HPTask\n");
    OS_TASK_Delay(50);
  }
}

/*********************************************************************
*
*       LPTask()
*/
static void LPTask(void) {
  while (1) {
    _Write("LPTask\n");
    OS_TASK_Delay(200);
  }
}

/*********************************************************************
*
*       main()
*/
int main(void) {
  OS_Init();                // Initialize embOS
  OS_InitHW();              // Initialize required hardware
  OS_TASK_CREATE(&TCBHP, "HP Task", 100, HPTask, StackHP);
  OS_TASK_CREATE(&TCBLP, "LP Task",  50, LPTask, StackLP);
  OS_MUTEX_Create(&Mutex);  // Creates mutex
  OS_Start();               // Start embOS
  return 0;
}

/*************************** End of file ****************************/
