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
File    : OS_Queues.c
Purpose : embOS sample program demonstrating the usage of queues.
*/

#include "RTOS.h"

/*********************************************************************
*
*       Defines
*
**********************************************************************
*/
#define MESSAGE_ALIGNMENT    (4u)  // Depends on core/compiler
#define MESSAGES_SIZE_HELLO  (7u + OS_Q_SIZEOF_HEADER + MESSAGE_ALIGNMENT)
#define MESSAGES_SIZE_WORLD  (9u + OS_Q_SIZEOF_HEADER + MESSAGE_ALIGNMENT)
#define QUEUE_SIZE           (MESSAGES_SIZE_HELLO + MESSAGES_SIZE_WORLD)

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/
static OS_STACKPTR int StackHP[128], StackLP[128];  // Task stacks
static OS_TASK         TCBHP, TCBLP;                // Task-control-blocks
static OS_QUEUE        MyQueue;
static char            MyQBuffer[QUEUE_SIZE];

/*********************************************************************
*
*       Local functions
*
**********************************************************************
*/

/*********************************************************************
*
*       HPTask()
*/
static void HPTask(void) {
  char* pData;
  int   Len;

  while (1) {
    Len = OS_QUEUE_GetPtrBlocked(&MyQueue, (void**)&pData);
    OS_TASK_Delay(10);
    //
    // Evaluate Message
    //
    if (Len) {
      OS_COM_SendString(pData);
      OS_QUEUE_Purge(&MyQueue);
    }
  }
}

/*********************************************************************
*
*       LPTask()
*/
static void LPTask(void) {
  while (1) {
    OS_QUEUE_Put(&MyQueue, "\nHello\0", 7);
    OS_QUEUE_Put(&MyQueue, "\nWorld !\0", 9);
    OS_TASK_Delay(500);
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
  OS_TASK_CREATE(&TCBHP, "HP Task", 100, HPTask, StackHP);
  OS_TASK_CREATE(&TCBLP, "LP Task",  50, LPTask, StackLP);
  OS_QUEUE_Create(&MyQueue, &MyQBuffer, sizeof(MyQBuffer));
  OS_COM_SendString("embOS OS_Queue example");
  OS_COM_SendString("\n\nDemonstrating message passing\n");
  OS_Start();   // Start embOS
  return 0;
}

/*************************** End of file ****************************/
