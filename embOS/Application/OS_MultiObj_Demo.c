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
File    : OS_MultiObj_Demo.c
Purpose : embOS sample program demonstrating multi object wait
*/

#include "RTOS.h"

/*********************************************************************
*
*       Static data
*/
static OS_STACKPTR int StackHP[128], StackLP[128];  // Task stacks
static OS_TASK         TCBHP, TCBLP;                // Task control blocks
static OS_QUEUE        MyQueue;                     // Queue control block
static char            MyQBuffer[30];               // Queue buffer
static OS_EVENT        MyEvent;                     // Event control block
//
// List of RTOS objects to simultaneously wait for
//
static const OS_MULTIOBJ_COND aMyMultiObjCond[] = {
  { &MyQueue, OS_MULTIOBJ_IsMessageInQueue, (void*)2    },
  { &MyEvent, OS_MULTIOBJ_IsEventSignaled , (void*)NULL },
};

static OS_MULTIOBJ aMyMultiObj[OS_COUNT_OF(aMyMultiObjCond)];  // Multiple object control blocks

/*********************************************************************
*
*       HPTask()
*/
static void HPTask(void) {
  OS_INT  Index;
  char*   pData;
  int     MessageCnt;
  OS_BOOL Signaled;

  while (1) {
    Index = OS_MULTIOBJ_WaitBlocked(aMyMultiObj, aMyMultiObjCond, OS_COUNT_OF(aMyMultiObjCond));
    switch (Index) {
    case 0:
      MessageCnt = OS_QUEUE_GetMessageCnt(&MyQueue);
      if (MessageCnt == 2) {
        OS_QUEUE_GetPtr(&MyQueue, (void**)&pData);
        OS_COM_SendString(pData);
        OS_QUEUE_Purge(&MyQueue);
        OS_QUEUE_GetPtr(&MyQueue, (void**)&pData);
        OS_COM_SendString(pData);
        OS_QUEUE_Purge(&MyQueue);
      }
      break;
    case 1:
      Signaled = OS_EVENT_Get(&MyEvent);
      if (Signaled != 0u) {
         OS_COM_SendString("\nEvent received.");
         OS_EVENT_Reset(&MyEvent);
      }
      break;
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
    OS_EVENT_Set(&MyEvent);
    OS_TASK_Delay(2);
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
  OS_QUEUE_Create(&MyQueue, MyQBuffer, sizeof(MyQBuffer));
  OS_EVENT_CreateEx(&MyEvent, OS_EVENT_RESET_MODE_MANUAL);
  OS_Start();   // Start embOS
  return 0;
}

/*************************** End of file ****************************/
