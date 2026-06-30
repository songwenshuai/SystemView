/*********************************************************************
*                    SEGGER Microcontroller GmbH                     *
*                        The Embedded Experts                        *
**********************************************************************
*                                                                    *
*            (c) 1995 - 2024 SEGGER Microcontroller GmbH             *
*                                                                    *
*       www.segger.com     Support: support@segger.com               *
*                                                                    *
**********************************************************************
*                                                                    *
*       SEGGER SystemView * Real-time application analysis           *
*                                                                    *
**********************************************************************
*                                                                    *
* All rights reserved.                                               *
*                                                                    *
* SEGGER strongly recommends to not make any changes                 *
* to or modify the source code of this software in order to stay     *
* compatible with the SystemView and RTT protocol, and J-Link.       *
*                                                                    *
* Redistribution and use in source and binary forms, with or         *
* without modification, are permitted provided that the following    *
* condition is met:                                                  *
*                                                                    *
* o Redistributions of source code must retain the above copyright   *
*   notice, this condition and the following disclaimer.             *
*                                                                    *
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND             *
* CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,        *
* INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF           *
* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE           *
* DISCLAIMED. IN NO EVENT SHALL SEGGER Microcontroller BE LIABLE FOR *
* ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR           *
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT  *
* OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;    *
* OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF      *
* LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT          *
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE  *
* USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH   *
* DAMAGE.                                                            *
*                                                                    *
**********************************************************************
-------------------------- END-OF-HEADER -----------------------------

File    : SEGGER_SYSVIEW_Config_embOS_Win32.c
Purpose : Sample setup configuration of SystemView with embOS.
Revision: $Rev: 28344 $
*/
#include "RTOS.h"
#include "SEGGER_SYSVIEW.h"
#include "SEGGER_SYSVIEW_Conf.h"
#include "SEGGER_SYSVIEW_embOS.h"
#include "SEGGER_SYSVIEW_Win32.h"

#define WIN32_LEAN_AND_MEAN
#include <stdio.h>
#include <string.h>
#include <Windows.h>

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
static U32               _TSDiv;
static CRITICAL_SECTION  _csLockString;
static char              _sISRNames[MAX_ISRNAMES_LENGTH];

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
  LARGE_INTEGER TSFreq;

  //
  // Get the performance counter frequency and scale it down to be < 1 GHz.
  // (We can only handle cycles >= 1ns)
  //
  QueryPerformanceFrequency(&TSFreq);
  _TSDiv = 1;
  if (TSFreq.QuadPart > 1000000000LL) {
    _TSDiv = (U32)(TSFreq.QuadPart / 1000000000LL);
    if (TSFreq.QuadPart % 1000000000LL) {
      _TSDiv++;
    }
    TSFreq.QuadPart /= _TSDiv;
  }
  SEGGER_SYSVIEW_Init(TSFreq.LowPart, TSFreq.LowPart,
                      &SYSVIEW_X_OS_TraceAPI, _cbSendSystemDesc);
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
  LARGE_INTEGER TS;

  QueryPerformanceCounter(&TS);
  if (_TSDiv > 1) {
    TS.QuadPart /= _TSDiv;
  }

  return TS.LowPart;
}

/*********************************************************************
*
*       SEGGER_SYSVIEW_X_GetInterruptId()
*
*  Function description
*    Get the "dummy" interrupt ID.
*/
U32 SEGGER_SYSVIEW_X_GetInterruptId(void) {
  return GetCurrentThreadId();
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
  static int CriticalSectionInitialized = 0;
  char s[100];

  //
  // Make sure the used critical section is initialized.
  //
  if (CriticalSectionInitialized == 0) {
    InitializeCriticalSection(&_csLockString);
    CriticalSectionInitialized = 1;
  }
  //
  // Check whether the string fits in the string buffer.
  //
  if (strlen(sName) < (sizeof(s) - 10)) {
    //
    // If this is the first entry we don't need the comma.
    //
    EnterCriticalSection(&_csLockString);
    if (strlen(_sISRNames) == 0) {
      sprintf(s, "I#%u=%s", (unsigned int)GetCurrentThreadId(), sName);
    } else {
      sprintf(s, ",I#%u=%s", (unsigned int)GetCurrentThreadId(), sName);
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
    LeaveCriticalSection(&_csLockString);
  }
}

/*************************** End of file ****************************/
