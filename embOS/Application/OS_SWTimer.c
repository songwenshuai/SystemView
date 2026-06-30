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
File    : OS_SWTimer.c
Purpose : embOS sample program running two simple software timer.
*/

#include "RTOS.h"
#include "BSP.h"

static OS_TIMER TIMER50, TIMER200;

static void Timer50(void) {
  BSP_ToggleLED(0);
  OS_TIMER_Restart(&TIMER50);
}

static void Timer200(void) {
  BSP_ToggleLED(1);
  OS_TIMER_Restart(&TIMER200);
}

/*********************************************************************
*
*       main()
*/
int main(void) {
  OS_Init();    // Initialize embOS
  OS_InitHW();  // Initialize required hardware
  BSP_Init();   // Initialize LED ports
  OS_TIMER_Create(&TIMER50,  Timer50,   50);
  OS_TIMER_Create(&TIMER200, Timer200, 200);
  OS_TIMER_Start(&TIMER50);
  OS_TIMER_Start(&TIMER200);
  OS_Start();   // Start embOS
  return 0;
}

/*************************** End of file ****************************/
