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
File    : OS_MeasureCST_Scope.c
Purpose : embOS sample program allowing measurement of the embOS
          context switching time by using an oscilloscope. To do so,
          it sets and clears a port pin (as defined in BSP.c). The
          context switching time is

            Time = (d - c) - (b - a)

             -----   --                   ---------------
                  | |  |                 |
                   -    -----------------
                  ^ ^  ^                 ^
                  a b  c                 d

          The time between c and d is the context switching time, but
          note that the real context switching time is shorter, as the
          signal also contains the overhead of switching the LED on
          and off. The time of this overhead is visible on an
          oscilloscope as a small peak between a and b.
*/

#include "RTOS.h"
#include "BSP.h"

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

static OS_STACKPTR int StackHP[128], StackLP[128];  // Task stacks
static OS_TASK         TCBHP, TCBLP;                // Task control blocks

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
  while (1) {
    OS_TASK_Suspend(NULL);  // Suspend high priority task
    BSP_ClrLED(0);          // Stop measurement
  }
}

/*********************************************************************
*
*       LPTask()
*/
static void LPTask(void) {
  while (1) {
    OS_TASK_Delay(100);      // Synchronize to tick to avoid jitter
    //
    // Display overhead measurement
    //
    BSP_SetLED(0);
    BSP_ClrLED(0);
    //
    // Perform measurement
    //
    BSP_SetLED(0);           // Start measurement
    OS_TASK_Resume(&TCBHP);  // Resume high priority task to force task switch
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
  BSP_Init();   // Initialize LED ports
  OS_TASK_CREATE(&TCBHP, "HP Task", 100, HPTask, StackHP);
  OS_TASK_CREATE(&TCBLP, "LP Task",  50, LPTask, StackLP);
  OS_Start();   // Start embOS
  return 0;
}

/*************************** End of file ****************************/
