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
File    : OS_Mailboxes.c
Purpose : embOS sample program demonstrating the usage of mailboxes.
*/

#include "RTOS.h"

/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/

#define MAX_MSG_SIZE  (8)  // Max. number of bytes per message
#define MAX_MSG_NUM   (2)  // Max. number of messages per Mailbox

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

static OS_STACKPTR int StackHP[128], StackLP[128];  // Task stacks
static OS_TASK         TCBHP, TCBLP;                // Task control blocks
static OS_MAILBOX      MyMailbox;
static char            MyMailboxBuffer[MAX_MSG_SIZE * MAX_MSG_NUM];

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
  char aData[MAX_MSG_SIZE];

  while (1) {
    OS_MAILBOX_GetBlocked(&MyMailbox, (void *)aData);
    OS_COM_SendString(aData);
  }
}

/*********************************************************************
*
*       LPTask()
*/
static void LPTask(void) {
  while (1) {
    OS_MAILBOX_PutBlocked(&MyMailbox, "\nHello\0");
    OS_MAILBOX_PutBlocked(&MyMailbox, "\nWorld!\0");
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
  OS_MAILBOX_Create(&MyMailbox, MAX_MSG_SIZE, MAX_MSG_NUM, &MyMailboxBuffer);
  OS_COM_SendString("embOS OS_Mailbox example");
  OS_COM_SendString("\n\nDemonstrating message passing\n");
  OS_Start();   // Start embOS
  return 0;
}

/*************************** End of file ****************************/
