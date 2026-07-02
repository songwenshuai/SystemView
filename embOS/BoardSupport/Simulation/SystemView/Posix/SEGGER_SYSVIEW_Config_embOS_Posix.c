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
File    : SEGGER_SYSVIEW_Config_embOS_Posix.c
Purpose : Sample setup configuration of SystemView with embOS.
*/

#define _GNU_SOURCE

#include "RTOS.h"
#include "SEGGER_SYSVIEW.h"
#include "SEGGER_SYSVIEW_Conf.h"
#include "SEGGER_SYSVIEW_embOS.h"
#include "SEGGER_SYSVIEW_Posix.h"

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/
// The application name to be displayed in SystemViewer
#ifndef   SYSVIEW_APP_NAME
  #define SYSVIEW_APP_NAME        "embOS start project"
#endif

// The target device name
#ifndef   SYSVIEW_DEVICE_NAME
  #define SYSVIEW_DEVICE_NAME     "Simulation"
#endif

// Frequency of the timestamp. Must match SEGGER_SYSVIEW_Conf.h
#ifndef   SYSVIEW_TIMESTAMP_FREQ
  #define SYSVIEW_TIMESTAMP_FREQ  (1000u)
#endif

// System Frequency. SystemcoreClock is used in most CMSIS compatible projects.
#ifndef   SYSVIEW_CPU_FREQ
  #define SYSVIEW_CPU_FREQ        (1000000u)
#endif

// Define as 1 to immediately start recording after initialization to catch system initialization.
#ifndef   SYSVIEW_START_ON_INIT
  #define SYSVIEW_START_ON_INIT   0
#endif

#ifndef   MAX_ISRNAMES_LENGTH
  #define MAX_ISRNAMES_LENGTH     400
#endif

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/
static char _sISRNames[MAX_ISRNAMES_LENGTH];

/*********************************************************************
*
*       Local functions, SystemView
*
**********************************************************************
*/

/*********************************************************************
*
*       _cbSendSystemDesc()
*
*  Function description
*    Sends SystemView description strings.
*/
static void _cbSendSystemDesc(void) {
  SEGGER_SYSVIEW_SendSysDesc("N=" SYSVIEW_APP_NAME ",O=embOS,D=" SYSVIEW_DEVICE_NAME );
  if (strlen(_sISRNames) > 0) {
    SEGGER_SYSVIEW_SendSysDesc(_sISRNames);
  }
}

/*********************************************************************
*
*       _GetCurrentThreadId()
*
*  Function description
*    Returns the shrunken current POSIX thread ID for SystemView ISR events.
*/
static U32 _GetCurrentThreadId(void) {
  return (U32)SEGGER_SYSVIEW_ShrinkId(SEGGER_PTR2ADDR(pthread_self()));
}

/*********************************************************************
*
*       Global functions
*
**********************************************************************
*/

/*********************************************************************
*
*       SEGGER_SYSVIEW_Conf()
*
*  Function description
*    Configure SystemView and embOS for use of SystemView.
*/
void SEGGER_SYSVIEW_Conf(void) {
  SEGGER_SYSVIEW_Init(1000000000, 1000000, &SYSVIEW_X_OS_TraceAPI, _cbSendSystemDesc);
  OS_SetTraceAPI(&embOS_TraceAPI_SYSVIEW);    // Configure embOS to use SYSVIEW.
#if SYSVIEW_START_ON_INIT
  SEGGER_SYSVIEW_Start();                     // Start recording to catch system initialization.
#endif
}

/*********************************************************************
*
*       SEGGER_SYSVIEW_X_GetTimestamp()
*
*  Function description
*    Get the timestamp for SystemView.
*    On Windows use the performance counter.
*/
U32 SEGGER_SYSVIEW_X_GetTimestamp(void) {
  struct timespec Timespec;
  OS_U64          Timestamp;

  clock_gettime(CLOCK_MONOTONIC, &Timespec);
  Timestamp  = Timespec.tv_sec * 1000000000;
  Timestamp += Timespec.tv_nsec;
  return Timestamp;
}

/*********************************************************************
*
*       SEGGER_SYSVIEW_X_GetInterruptId()
*
*  Function description
*    Get the "dummy" interrupt ID.
*/
U32 SEGGER_SYSVIEW_X_GetInterruptId(void) {
  return _GetCurrentThreadId();
}

/*********************************************************************
*
*       SEGGER_SYSVIEW_X_SetISRName()
*
*  Function description
*    Informs SystemView about an ISR name.
*
*  Parameters
*    sName: ISR Name
*
*  Additional information
*    Must be called from an ISR after SEGGER_SYSVIEW_Conf() only.
*    It uses the thread ID as an unique ID for SystemView.
*/
void SEGGER_SYSVIEW_X_SetISRName(const char* sName) {
  char s[100];

  //
  // Check whether the string fits in the string buffer.
  //
  if (strlen(sName) < (sizeof(s) - 15u)) {
    OS_INT_IncDI();
    //
    // If this is the first entry we don't need the comma.
    //
    if (strlen(_sISRNames) == 0) {
      sprintf(s, "I#%u=%s", _GetCurrentThreadId(), sName);
    } else {
      sprintf(s, ",I#%u=%s", _GetCurrentThreadId(), sName);
    }
    //
    // Add new ISR name to the ISR name string and inform SystemView (if enough space is left in the string buffer).
    //
    if ((strlen(_sISRNames) + strlen(s) + 1) < MAX_ISRNAMES_LENGTH) {
      strcat(_sISRNames, s);
      //
      // Send new description if SystemView is started.
      //
      if (SEGGER_SYSVIEW_IsStarted() > 0) {
        SEGGER_SYSVIEW_SendSysDesc(_sISRNames);
      }
    }
    OS_INT_DecRI();
  }
}

/*************************** End of file ****************************/
