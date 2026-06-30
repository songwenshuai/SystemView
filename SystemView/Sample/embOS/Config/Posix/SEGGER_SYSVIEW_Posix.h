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
File    : SEGGER_SYSVIEW_Posix.h
Purpose : System visualization API.
*/

#ifndef SEGGER_SYSVIEW_POSIX_H
#define SEGGER_SYSVIEW_POSIX_H

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include "SEGGER_SYSVIEW.h"

/*********************************************************************
*
*       Prototypes
*
**********************************************************************
*/
#ifdef __cplusplus
extern "C" {
#endif

void SEGGER_SYSVIEW_X_SetISRName(const char* sName);

#ifdef __cplusplus
}
#endif

#endif

/*************************** End of file ****************************/
