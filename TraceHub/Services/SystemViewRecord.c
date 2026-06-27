/*********************************************************************
*                             CineLogic                              *
*                      RTT Trace and Debug Bridge                    *
**********************************************************************
----------------------------------------------------------------------
File    : SystemViewRecord.c
Purpose : SystemView recording file runtime
---------------------------END-OF-HEADER------------------------------
*/

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "SystemView_internal.h"
#include "Log.h"
#include "RTTBridge.h"

/*********************************************************************
*
*       _SystemView_ReportRecordFileError()
*
*  Function description
*    Mark record file failure as fatal and stop the service.
*/
void _SystemView_ReportRecordFileError(SystemView_State_t *pState,
                                       const char *operation,
                                       int saved_errno) {
    bool should_report;

    if (pState == NULL) {
        return;
    }

    if (operation == NULL) {
        operation = "operation";
    }

    should_report = false;
    if (pState->LockInitialized) {
        SYS_MutexLock(&pState->Lock);
    }
    if (!pState->RecordFileError) {
        pState->RecordFileError = true;
        pState->FatalError = true;
        pState->Running = false;
        should_report = true;
    } else {
        pState->Running = false;
    }
    if (pState->LockInitialized) {
        SYS_MutexUnlock(&pState->Lock);
    }

    if (should_report) {
        if (saved_errno != 0) {
            Log_Error("SystemView: record file %s failed: %s\n",
                      operation,
                      strerror(saved_errno));
        } else {
            Log_Error("SystemView: record file %s failed\n", operation);
        }
    }
}

/*********************************************************************
*
*       _SystemView_FlushRecordFile()
*
*  Function description
*    Flush the SystemView record file and report persistence errors.
*/
int _SystemView_FlushRecordFile(SystemView_State_t *pState, FILE *record_file) {
    if (record_file == NULL) {
        return 0;
    }

    errno = 0;
    if (fflush(record_file) != 0) {
        _SystemView_ReportRecordFileError(pState, "flush", errno);
        return -1;
    }

    return 0;
}

/*********************************************************************
*
*       _SystemView_RecordingThread()
*
*  Function description
*    Recording thread for SystemView data.
*    Reads RTT data, records to file, and feeds the network queue.
*
*  Parameters
*    pArg - Unused parameter (required by thread API)
*/
void _SystemView_RecordingThread(void *pArg) {
    (void)pArg;

    SystemView_State_t *pState = &_sysview_state;
    uintptr_t           Address;
    int                 NumBytes;
    FILE               *record_file;
    char                Buffer[SYSVIEW_SEND_BUF_SIZE];
    unsigned            Now;
    unsigned            LastRecordFlushTick;
    size_t              Written;
    size_t              PendingRecordBytes;

    Log_Print("SystemView recording thread started, channel %u\n",
              pState->Config.channel);

    LastRecordFlushTick = SYS_GetTickCount();
    PendingRecordBytes = 0u;

    while (_SystemView_IsRunning(pState)) {
        record_file = pState->record_file;
        if (RTTBridge_GetValidatedRTTRegion(&Address, NULL) != 0) {
            _SystemView_ReportRecoverableRTTError(pState, "region validation");
            SYS_Sleep(SYSVIEW_RECOVERY_DELAY_MS);
            continue;
        }

        //
        // Read data from RTT buffer
        //
        NumBytes = RTTBridge_ReadUpBufferNoLock(pState->Config.channel, Buffer, sizeof(Buffer));

        if (NumBytes >= 0) {
            _SystemView_ReportRTTRecovered(pState);
        }
        if (NumBytes > 0) {
            //
            // Record to file if available
            //
            if (record_file != NULL) {
                errno = 0;
                Written = fwrite(Buffer, 1, (size_t)NumBytes, record_file);
                if (Written != (size_t)NumBytes) {
                    _SystemView_ReportRecordFileError(pState, "write", errno);
                    continue;
                }
                PendingRecordBytes += Written;
                Now = SYS_GetTickCount();
                if ((PendingRecordBytes >= SYSVIEW_RECORD_FLUSH_THRESHOLD) ||
                    ((unsigned)(Now - LastRecordFlushTick) >= SYSVIEW_RECORD_FLUSH_INTERVAL_MS)) {
                    if (_SystemView_FlushRecordFile(pState, record_file) != 0) {
                        break;
                    }
                    LastRecordFlushTick = Now;
                    PendingRecordBytes = 0u;
                }
            }
            if (_SystemView_QueueTargetData(pState, Buffer, (unsigned)NumBytes) != 0) {
                break;
            }
        } else if (NumBytes < 0) {
            _SystemView_ReportRecoverableRTTError(pState, "up-buffer read");
            SYS_Sleep(SYSVIEW_RECOVERY_DELAY_MS);
            continue;
        } else {
            //
            // No data available, sleep to reduce CPU usage
            //
            SYS_Sleep(SYSVIEW_IDLE_DELAY_MS);
        }
    }

    record_file = pState->record_file;
    if ((record_file != NULL) && (PendingRecordBytes > 0u)) {
        (void)_SystemView_FlushRecordFile(pState, record_file);
    }

    Log_Print("SystemView recording thread stopped\n");
}


/*************************** End of file ****************************/
