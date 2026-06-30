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
File    : JLINKMEM.h
Purpose : Header file for J-Link ARM communication using memory
*/

#ifndef JLINKMEM_H
#define JLINKMEM_H

#ifdef __cplusplus
extern "C" {
#endif

void JLINKMEM_Process         (void);
void JLINKMEM_SetpfOnRx       (void (*pfOnRx)(OS_U8 Data));
void JLINKMEM_SetpfOnTx       (OS_U8 (*pfOnTx)(void));
void JLINKMEM_SetpfGetNextChar(OS_INT (*pfGetNextChar)(void));
void JLINKMEM_SendChar        (OS_U8 Data);

#ifdef __cplusplus
}
#endif

#endif  // JLINKMEM_H

/*************************** End of file ****************************/
