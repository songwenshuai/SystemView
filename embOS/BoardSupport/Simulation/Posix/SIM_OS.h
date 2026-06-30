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
File    : SIM_OS.h
Purpose : Function declarations for GUI simulation
*/

#ifndef SIM_OS_H
#define SIM_OS_H

#include "RTOS.h"

#if defined(__cplusplus)
  extern "C" {                // Make sure we have C-declarations in C++ programs.
#endif

/*********************************************************************
*
*        Compatibility with older embOS simulation versions
*
**********************************************************************
*/

#define SIM_Init(x)  SIM_OS_InitWindow()
#define SIM_Update   SIM_OS_UpdateWindow
#define SIM_Paint    SIM_OS_PaintWindow

/*********************************************************************
*
*       API functions
*
**********************************************************************
*/

int  SIM_OS_InitWindow         (void);
void SIM_OS_UpdateWindow       (void);
void SIM_OS_PaintWindow        (void* pContext);

#if defined(EMBOS_SIM_HOST_POSIX) && (EMBOS_SIM_HOST_POSIX != 0)
int  SIM_OS_RunWindow          (void);
void SIM_OS_RequestQuit        (void);
void SIM_OS_ShutdownWindow     (void);
void OS_SIM_RequestStop        (void);
#endif

#if defined(__cplusplus)
}                             // Make sure we have C-declarations in C++ programs.
#endif

#endif                        // Avoid multiple inclusion

/*************************** End of file ****************************/
