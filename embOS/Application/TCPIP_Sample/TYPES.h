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
File    : Types.h
Purpose : Global data types
*/

#ifndef TYPES_H            // Guard against multiple inclusion
#define TYPES_H

/*********************************************************************
*
*       Defines, basic types
*
**********************************************************************
*/

#define   U8    unsigned char
#define   U16   unsigned short
#define   U32   unsigned int
#ifdef _MSC_VER
  #define U64   unsigned __int64
#else
  #define U64   unsigned long long
#endif
#define   I8    signed char
#define   I16   signed short
#define   I32   signed int
#ifdef _MSC_VER
  #define I64   signed __int64
#else
  #define I64   signed long long
#endif

#endif                      // Avoid multiple inclusion

/*************************** End of file ****************************/
