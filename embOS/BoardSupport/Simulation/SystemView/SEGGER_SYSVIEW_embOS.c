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

File    : SEGGER_SYSVIEW_embOS.c
Purpose : Interface between embOS and System View.
Revision: $Rev: 25329 $
*/

#include "RTOS.h"
#include "SEGGER_SYSVIEW.h"
#include "SEGGER_RTT.h"
#include "SEGGER_SYSVIEW_embOS.h"

#include <string.h>

#if (OS_VERSION < 41201)
  #error "SystemView is only supported in embOS V4.12a and later."
#endif

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/
static const char* sDefaultTaskName = "n/a";

/*********************************************************************
*
*       Local functions
*
**********************************************************************
*/

/*********************************************************************
*
*       _cbSendTaskInfo()
*
*  Function description
*    Sends task information to SystemView
*/
static void _cbSendTaskInfo(const OS_TASK* pTask) {
  SEGGER_SYSVIEW_TASKINFO Info;

  OS_EnterRegion();                // No scheduling to make sure the task list does not change while we are transmitting it
  memset(&Info, 0, sizeof(Info));  // Fill all elements with 0 to allow extending the structure in future version without breaking the code
  Info.TaskID = SEGGER_PTR2ADDR(pTask);
#if (OS_TRACKNAME != 0)
#if (OS_VERSION >= 51600)
  Info.sName = pTask->sName;
#else
  Info.sName = pTask->Name;
#endif
#endif
  if (Info.sName == NULL) {
    Info.sName = sDefaultTaskName;
  }
  Info.Prio = pTask->Priority;
#if (OS_CHECKSTACK != 0)
  Info.StackBase = SEGGER_PTR2ADDR(pTask->pStackBase);
  Info.StackSize = pTask->StackSize;
#endif
  SEGGER_SYSVIEW_SendTaskInfo(&Info);
  OS_LeaveRegion();                // No scheduling to make sure the task list does not change while we are transmitting it
}

/*********************************************************************
*
*       _cbSendTaskList()
*
*  Function description
*    This function is part of the link between embOS and SYSVIEW.
*    Called from SystemView when asked by the host, it uses SYSVIEW
*    functions to send the entire task list to the host.
*/
static void _cbSendTaskList(void) {
  OS_TASK*    pTask;

  OS_EnterRegion();         // No scheduling to make sure the task list does not change while we are transmitting it
  for (pTask = OS_Global.pTask; pTask; pTask = pTask->pNext) {
    _cbSendTaskInfo(pTask);
  }
#if ((OS_VERSION >= 43800) && (OS_TRACKNAME != 0))  // Human readable object identifiers supported since embOS V4.38
  {
    OS_OBJNAME* p;
#if (OS_VERSION >= 51600)
    for (p = OS_Global.pObjNameRoot; p != NULL; p = p->pNext) {
#else
    for (p = OS_pObjNameRoot; p != NULL; p = p->pNext) {
#endif
      SEGGER_SYSVIEW_NameResource(SEGGER_PTR2ADDR(p->pOSObjID), p->sName);
    }
  }
#endif
  OS_LeaveRegion();         // No scheduling to make sure the task list does not change while we are transmitting it
}

/*********************************************************************
*
*       _cbRecordEnterISR()
*
*  Function description
*    Records ISR entry events.
*/
static void _cbRecordEnterISR(void) {
  SEGGER_SYSVIEW_RecordEnterISR();
}

/*********************************************************************
*
*       _cbRecordExitISR()
*
*  Function description
*    Records ISR exit events.
*/
static void _cbRecordExitISR(void) {
  SEGGER_SYSVIEW_RecordExitISR();
}

/*********************************************************************
*
*       _cbRecordExitISRToScheduler()
*
*  Function description
*    Records ISR exits that switch to the scheduler.
*/
static void _cbRecordExitISRToScheduler(void) {
  SEGGER_SYSVIEW_RecordExitISRToScheduler();
}

/*********************************************************************
*
*       _cbOnTaskCreate()
*
*  Function description
*    Records task creation events.
*/
static void _cbOnTaskCreate(OS_TRACE_ID_TYPE TaskId) {
  SEGGER_SYSVIEW_OnTaskCreate((PTR_ADDR)TaskId);
}

/*********************************************************************
*
*       _cbOnTaskStartExec()
*
*  Function description
*    Records task execution start events.
*/
static void _cbOnTaskStartExec(OS_TRACE_ID_TYPE TaskId) {
  SEGGER_SYSVIEW_OnTaskStartExec((PTR_ADDR)TaskId);
}

/*********************************************************************
*
*       _cbOnTaskStopExec()
*
*  Function description
*    Records task execution stop events.
*/
static void _cbOnTaskStopExec(void) {
  SEGGER_SYSVIEW_OnTaskStopExec();
}

/*********************************************************************
*
*       _cbOnTaskStartReady()
*
*  Function description
*    Records task ready start events.
*/
static void _cbOnTaskStartReady(OS_TRACE_ID_TYPE TaskId) {
  SEGGER_SYSVIEW_OnTaskStartReady((PTR_ADDR)TaskId);
}

/*********************************************************************
*
*       _cbOnTaskStopReady()
*
*  Function description
*    Records task ready stop events.
*/
static void _cbOnTaskStopReady(OS_TRACE_ID_TYPE TaskId, unsigned int Reason) {
  SEGGER_SYSVIEW_OnTaskStopReady((PTR_ADDR)TaskId, Reason);
}

/*********************************************************************
*
*       _cbOnIdle()
*
*  Function description
*    Records idle events.
*/
static void _cbOnIdle(void) {
  SEGGER_SYSVIEW_OnIdle();
}

/*********************************************************************
*
*       _cbOnTaskTerminate()
*
*  Function description
*    Binds the embOS task-terminate trace callback.
*/
#define _cbOnTaskTerminate  SEGGER_SYSVIEW_OnTaskTerminate

//
// embOS trace API that targets SYSVIEW
//
const OS_TRACE_API embOS_TraceAPI_SYSVIEW = {
  //
  // Specific Trace Events
  //
  _cbRecordEnterISR,                            //  void (*pfRecordEnterISR)              (void);
  _cbRecordExitISR,                             //  void (*pfRecordExitISR)               (void);
  _cbRecordExitISRToScheduler,                  //  void (*pfRecordExitISRToScheduler)    (void);
  _cbSendTaskInfo,                              //  void (*pfRecordTaskInfo)              (const OS_TASK* pTask);
  _cbOnTaskCreate,                              //  void (*pfRecordTaskCreate)            (OS_TRACE_ID_TYPE TaskId);
  _cbOnTaskStartExec,                           //  void (*pfRecordTaskStartExec)         (OS_TRACE_ID_TYPE TaskId);
  _cbOnTaskStopExec,                            //  void (*pfRecordTaskStopExec)          (void);
  _cbOnTaskStartReady,                          //  void (*pfRecordTaskStartReady)        (OS_TRACE_ID_TYPE TaskId);
  _cbOnTaskStopReady,                           //  void (*pfRecordTaskStopReady)         (OS_TRACE_ID_TYPE TaskId, unsigned Reason);
  _cbOnIdle,                                    //  void (*pfRecordIdle)                  (void);
  //
  // Generic Trace Event logging
  //
  SEGGER_SYSVIEW_RecordVoid,                    //  void    (*pfRecordVoid)               (unsigned Id);
  SEGGER_SYSVIEW_RecordU32,                     //  void    (*pfRecordU32)                (unsigned Id, OS_U32 Para0);
  SEGGER_SYSVIEW_RecordU32x2,                   //  void    (*pfRecordU32x2)              (unsigned Id, OS_U32 Para0, OS_U32 Para1);
  SEGGER_SYSVIEW_RecordU32x3,                   //  void    (*pfRecordU32x3)              (unsigned Id, OS_U32 Para0, OS_U32 Para1, OS_U32 Para2);
  SEGGER_SYSVIEW_RecordU32x4,                   //  void    (*pfRecordU32x4)              (unsigned Id, OS_U32 Para0, OS_U32 Para1, OS_U32 Para2, OS_U32 Para3);
  SEGGER_SYSVIEW_RecordId,                      //  void    (*pfRecordId)                 (unsigned Id, OS_TRACE_ID_TYPE Para0);
  SEGGER_SYSVIEW_RecordIdxU32,                  //  void    (*pfRecordIdxU32)             (unsigned Id, OS_TRACE_ID_TYPE Para0, OS_U32 Para1);
  SEGGER_SYSVIEW_RecordIdxU32x2,                //  void    (*pfRecordIdxU32x2)           (unsigned Id, OS_TRACE_ID_TYPE Para0, OS_U32 Para1, OS_U32 Para2);
  SEGGER_SYSVIEW_RecordIdxU32x3,                //  void    (*pfRecordIdxU32x3)           (unsigned Id, OS_TRACE_ID_TYPE Para0, OS_U32 Para1, OS_U32 Para2, OS_U32 Para3);
  SEGGER_SYSVIEW_RecordIdxU32x4,                //  void    (*pfRecordIdxU32x4)           (unsigned Id, OS_TRACE_ID_TYPE Para0, OS_U32 Para1, OS_U32 Para2, OS_U32 Para3, OS_U32 Para4);
  SEGGER_SYSVIEW_ShrinkId,                      //  OS_TRACE_ID_TYPE (*pfPtrToId)         (OS_TRACE_ID_TYPE Ptr);
#if (OS_VERSION >= 41400)  // Tracing timer is supported since embOS V4.14
  SEGGER_SYSVIEW_RecordEnterTimer,              //  void    (*pfRecordEnterTimer)         (OS_TRACE_ID_TYPE TimerID);
  SEGGER_SYSVIEW_RecordExitTimer,               //  void    (*pfRecordExitTimer)          (void);
#endif
#if (OS_VERSION >= 42400)   // Tracing end of call supported since embOS V4.24
  SEGGER_SYSVIEW_RecordEndCall,                 //  void    (*pfRecordEndCall)            (unsigned int Id);
  SEGGER_SYSVIEW_RecordEndCallU32,              //  void    (*pfRecordEndCallReturnValue) (unsigned int Id, OS_U32 ReturnValue);
  SEGGER_SYSVIEW_RecordEndCallId,               //  void    (*pfRecordEndCallId)          (unsigned int Id, OS_TRACE_ID_TYPE ReturnValue);
  _cbOnTaskTerminate,                           //  void    (*pfRecordTaskTerminate)      (OS_TRACE_ID_TYPE TaskId);
  SEGGER_SYSVIEW_RecordU32x5,                   //  void    (*pfRecordU32x5)              (unsigned Id, OS_U32 Para0, OS_U32 Para1, OS_U32 Para2, OS_U32 Para3, OS_U32 Para4);
#endif
#if (OS_VERSION >= 43800)  // Human readable object identifiers supported since embOS V4.38
  SEGGER_SYSVIEW_NameResource,                  // void  (*pfRecordObjName)               (OS_TRACE_ID_TYPE Id, OS_CONST_PTR char* Para0);
#endif
};

//
// Services provided to SYSVIEW by embOS
//
const SEGGER_SYSVIEW_OS_API SYSVIEW_X_OS_TraceAPI = {
  OS_GetTime_us64,
  _cbSendTaskList,
};

/*************************** End of file ****************************/
