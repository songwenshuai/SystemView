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
File    : OS_ExtendedTask.c
Purpose : embOS sample program demonstrating the extension of tasks.
          This sample application is described in the generic manual
          in chapter:
          "Extending the task context by using own task structures"
*/

#include "RTOS.h"

/*********************************************************************
*
*       Types, local
*
**********************************************************************
*/

//
// Custom task structure with extended task context.
//
typedef struct {
  OS_TASK Task;     // OS_TASK has to be the first element
  OS_TIME Timeout;  // Any other data type may be used to extend the context
  char*   pString;  // Any number of elements may be used to extend the context
} MY_APP_TASK;

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/
static OS_STACKPTR int StackHP[128], StackLP[128];  // Task stacks
static MY_APP_TASK     TCBHP, TCBLP;                // Task-control-blocks

/*********************************************************************
*
*       Local functions
*
**********************************************************************
*/

/*********************************************************************
*
*       MyTask()
*/
static void MyTask(void) {
  MY_APP_TASK* pThis;
  OS_TIME      Timeout;
  char*        pString;

  pThis = (MY_APP_TASK*)OS_TASK_GetID();
  while (1) {
    Timeout = pThis->Timeout;
    pString = pThis->pString;
    OS_COM_SendString(pString);
    OS_TASK_Delay(Timeout);
  }
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
*/
int main(void) {
  OS_Init();    // Initialize embOS
  OS_InitHW();  // Initialize required hardware
  //
  // Create the extended tasks just as normal tasks.
  // Note that the first parameter has to be of type OS_TASK
  //
  OS_TASK_CREATE(&TCBHP.Task, "HP Task", 100, MyTask, StackHP);
  OS_TASK_CREATE(&TCBLP.Task, "LP Task",  50, MyTask, StackLP);
  //
  // Give task contexts individual data
  //
  TCBHP.Timeout = 200;
  TCBHP.pString = "HP task running\n";
  TCBLP.Timeout = 500;
  TCBLP.pString = "LP task running\n";
  OS_Start();   // Start embOS
  return 0;
}

/*************************** End of file ****************************/
