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
File    : GLOBAL.h
Purpose : Global types etc.
*/

#ifndef GLOBAL_H            // Guard against multiple inclusion
#define GLOBAL_H

#include <memory.h>         // For memset
#include "TYPES.h"          // Defines standard data types

/*********************************************************************
*
*       Defines, function replacements
*
**********************************************************************
*/

#ifndef   COUNTOF
  #define COUNTOF(a)    (sizeof(a)/sizeof(a[0]))
#endif

#ifndef   ZEROFILL
  #define ZEROFILL(Obj) memset(&Obj, 0, sizeof(Obj))
#endif

#ifndef   LIMIT
  #define LIMIT(a,b)    if (a > b) a = b;
#endif

#ifndef   MIN
  #define MIN(a, b)     (((a) < (b)) ? (a) : (b))
#endif

#ifndef   MAX
  #define MAX(a, b)     (((a) > (b)) ? (a) : (b))
#endif

/*********************************************************************
*
*       Types, internal
*
**********************************************************************
*/

typedef enum {IS_NOINIT, IS_RUNNING, IS_EXIT} INIT_STATE;

#endif                      // Avoid multiple inclusion

/*************************** End of file ****************************/
