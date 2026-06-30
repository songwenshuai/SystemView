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
File    : OS_Config.h
Purpose : Configuration settings for the OS build and embOSView
*/

#ifndef OS_CONFIG_H
#define OS_CONFIG_H

/*********************************************************************
*
*       Configuration for RTOS build and embOSView communication
*
*  In your application program, you need to let the compiler know
*  which build of embOS you are using. This is done by adding the
*  corresponding OS_LIBMODE_* define to your preprocessor settings
*  and linking the appropriate embOS library (or embOS sources).
*
*  Name / Define    | Debug Code | Stack Check | Profiling | embOS API Trace | embOSView API Trace | Round-Robin | Object Names | Task Context Extension
*  -----------------------------------------------------------------------------------------------------------------------------------------------------
*  OS_LIBMODE_XR    |            |             |           |                 |                     |             |              |
*  OS_LIBMODE_R     |            |             |           |                 |                     | X           | X            | X
*  OS_LIBMODE_S     |            | X           |           |                 |                     | X           | X            | X
*  OS_LIBMODE_SP    |            | X           | X         | X               |                     | X           | X            | X
*  OS_LIBMODE_D     | X          | X           |           |                 |                     | X           | X            | X
*  OS_LIBMODE_DP    | X          | X           | X         | X               |                     | X           | X            | X
*  OS_LIBMODE_DT    | X          | X           | X         | X               | X                   | X           | X            | X
*  OS_LIBMODE_SAFE  | X          | X           | X         | X               |                     | X           | X            | X
*
*  If no OS_LIBMODE_* setting is used, this file will select default
*  library modes for debug and release compile configurations of your project
*  (assuming DEBUG is defined to 1 in debug compile configuration).
*/

#if !defined(OS_LIBMODE_XR) \
 && !defined(OS_LIBMODE_R)  \
 && !defined(OS_LIBMODE_S)  \
 && !defined(OS_LIBMODE_SP) \
 && !defined(OS_LIBMODE_D)  \
 && !defined(OS_LIBMODE_DP) \
 && !defined(OS_LIBMODE_DT) \
 && !defined(OS_LIBMODE_SAFE)
  #if (defined(DEBUG) && (DEBUG == 1))
    #define OS_LIBMODE_DP
  #else
    #define OS_LIBMODE_R
  #endif
#endif

#if (defined(OS_LIBMODE_R) || defined(OS_LIBMODE_XR)) && !defined(OS_VIEW_IFSELECT)
  #define OS_VIEW_IFSELECT  OS_VIEW_DISABLED  // embOSView communication is disabled per default in release configuration
#endif

/*********************************************************************
*
*  Additional embOS compile time configuration defines when using
*  embOS sources in your project or rebuilding the embOS libraries
*  can be added here, e.g.:
*    #define OS_SUPPORT_TICKLESS  0  // Disable tickless support
*/

#endif  // OS_CONFIG_H

/*************************** End of file ****************************/
