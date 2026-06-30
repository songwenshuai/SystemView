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
File    : OS_Semaphores.c
Purpose : The HookRoutine signals the arrival of new data to a worker 
          task, which subsequently processes that data. 
          When the worker task is blocked from execution, e.g. by a
          higher-priority task, the semaphore's counter effectively tracks
          the number of data packets to be processed by the worker task,
          which will be executed for that exact number of times when resumed.
*/

#include "RTOS.h"

static OS_STACKPTR int Stack[128];  // Task stack
static OS_TASK         TCB;         // Task control block
static OS_SEMAPHORE    Sema;        // Semaphore
static OS_TICK_HOOK    Hook;        // Tick Hook

static void WorkerTask(void) {
  while(1) {
    OS_SEMAPHORE_TakeBlocked(&Sema);         // Wait for signaling of received data
    OS_COM_SendString("Processing data\n");  // Act on received data
  }
}

static void HookRoutine(void) {
  OS_SEMAPHORE_Give(&Sema);                  // Signal data reception
}

/*********************************************************************
*
*       main()
*/
int main(void) {
  OS_Init();                              // Initialize embOS
  OS_InitHW();                            // Initialize required hardware
  OS_TASK_CREATE(&TCB, "Worker Task", 100, WorkerTask, Stack);
  OS_SEMAPHORE_Create(&Sema, 0);          // Creates semaphore
  OS_TICK_AddHook(&Hook, HookRoutine);    // Add hook routine to system tick
  OS_Start();                             // Start embOS
  return 0;
}

/*************************** End of file ****************************/
