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
File    : OS_ReadersWriterLock.c
Purpose : embOS sample program for Readers-Writer Lock
*/

#include "RTOS.h"

#define NUM_READERS  2

static OS_STACKPTR int StackRd1[128], StackRd2[128], StackWr[128];
static OS_TASK         TCBRd1, TCBRd2, TCBWr;
static OS_RWLOCK       Lock;
static OS_U32          GlobalVar;

/*********************************************************************
*
*       Reader_Task()
*/
static void RdTask(void) {
  OS_U32 LocalVar;

  while (1) {
    OS_RWLOCK_RdLockBlocked(&Lock);
    LocalVar = GlobalVar;
    OS_RWLOCK_RdUnlock(&Lock);
    OS_USE_PARA(LocalVar);
  }
}

/*********************************************************************
*
*       Writer_Task()
*/
static void WrTask(void) {
  while (1) {
    OS_RWLOCK_WrLockBlocked(&Lock);
    GlobalVar++;
    OS_RWLOCK_WrUnlock(&Lock);
    OS_TASK_Delay(10);
  }
}

/*********************************************************************
*
*       main()
*/
int main(void) {
  OS_Init();    // Initialize embOS
  OS_InitHW();  // Initialize required hardware
  OS_TASK_CREATE(&TCBRd1, "Reader Task 1", 100, RdTask, StackRd1);
  OS_TASK_CREATE(&TCBRd2, "Reader Task 2", 100, RdTask, StackRd2);
  OS_TASK_CREATE(&TCBWr,  "Writer Task"  , 101, WrTask, StackWr);
  OS_RWLOCK_Create(&Lock, NUM_READERS);
  OS_Start();   // Start embOS
  return 0;
}

/*************************** End of file ****************************/
