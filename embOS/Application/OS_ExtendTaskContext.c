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
File    : OS_ExtendTaskContext.c
Purpose : embOS sample program demonstrating the dynamic extension of
          tasks' contexts. This is done by adding a global variable to
          the task context of certain tasks.
*/

#include "RTOS.h"

/*********************************************************************
*
*       Types, local
*
**********************************************************************
*/

//
// Custom structure with task context extension.
// In this case, the extended task context consists of just
// a single member, which is a global variable.
//
typedef struct {
  int GlobalVar;
} CONTEXT_EXTENSION;

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

static OS_STACKPTR int StackHP[128], StackLP[128];  // Task stacks
static OS_TASK         TCBHP, TCBLP;                // Task control blocks
static int             GlobalVar;

/*********************************************************************
*
*       Local functions
*
**********************************************************************
*/

/*********************************************************************
*
*       _Save()
*
*  Function description
*    Saves GlobalVar on the task stack
*/
static void OS_STACKPTR* _Save(void OS_STACKPTR* pStack) {
  CONTEXT_EXTENSION* p;

  p = (CONTEXT_EXTENSION*)pStack;
#if (OS_STACK_GROWS_TOWARD_HIGHER_ADDR == 1)
  p++;
#else
  p--;
#endif
  p->GlobalVar = GlobalVar;
  return (void OS_STACKPTR*)p;
}

/*********************************************************************
*
*       _Restore()
*
*  Function description
*    Restores GlobalVar from the task stack
*/
static void OS_STACKPTR* _Restore(const void OS_STACKPTR* pStack) {
  const CONTEXT_EXTENSION* p;

  p = (CONTEXT_EXTENSION*)pStack;
#if (OS_STACK_GROWS_TOWARD_HIGHER_ADDR == 1)
  p++;
#else
  p--;
#endif
  GlobalVar = p->GlobalVar;
  return (void OS_STACKPTR*)p;
}

/*********************************************************************
*
*       Public API structure
*/
const OS_EXTEND_TASK_CONTEXT _SaveRestore = {
  _Save,    // Function pointer to save the task context
  _Restore  // Function pointer to restore the task context
};

/*********************************************************************
*
*       HPTask()
*
*  Function description
*    During the execution of this function, the thread-specific
*    global variable GlobalVar always has the same value of 1.
*/
static void HPTask(void) {
  OS_TASK_SetContextExtension(&_SaveRestore);
  GlobalVar = 1;
  while (1) {
    OS_TASK_Delay(10);
  }
}

/*********************************************************************
*
*       LPTask()
*
*  Function description
*    During the execution of this function, the thread-specific
*    global variable GlobalVar always has the same value of 2.
*/
static void LPTask(void) {
  OS_TASK_SetContextExtension(&_SaveRestore);
  GlobalVar = 2;
  while (1) {
    OS_TASK_Delay(50);
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
  OS_Start();   // Start embOS
  return 0;
}

/*************************** End of file ****************************/
