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
File    : OS_Start2Tasks.cpp
Purpose : embOS C++ sample program running two simple tasks.
*/

#include "RTOS.h"
#include "stdlib.h"

/*********************************************************************
*
*       Class definition
*
**********************************************************************
*/
class CThread:private OS_TASK {
public:
  CThread(char* sTaskName, unsigned int TaskPriority, unsigned int TaskDelay) {
    void* pTaskStack = malloc(256u);
    if (pTaskStack != NULL) {
      OS_TASK_CreateEx(this, sTaskName, TaskPriority, CThread::run, reinterpret_cast<OS_STACKPTR void*>(pTaskStack), 256u, 2u, reinterpret_cast<void*>(TaskDelay));
    }
  }

private:
  static void run(void* t) {
    while (1) {
      OS_TASK_Delay(reinterpret_cast<unsigned long>(t));
    }
  }
};

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/
CThread *HPTask, *LPTask;

/*********************************************************************
*
*       Global functions
*
**********************************************************************
*/
int main(void) {
  OS_Init();    // Initialize embOS
  OS_InitHW();  // Initialize required hardware
  HPTask = new CThread(const_cast<char*>("HPTask"), 100u, 50u);
  LPTask = new CThread(const_cast<char*>("LPTask"), 50u, 200u);
  OS_Start();   // Start embOS
  return 0;
}

/*************************** End of file ****************************/
