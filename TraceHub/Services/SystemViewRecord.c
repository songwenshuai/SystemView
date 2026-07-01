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
#include <time.h>

#include "SystemView_internal.h"
#include "Log.h"
#include "RTTBridge.h"

/*********************************************************************
*
*       Static functions
*
**********************************************************************
*/

/*********************************************************************
*
*       _SystemView_FormatRecordTime()
*
*  Function description
*    Format the current local wall-clock time for the SVDat header.
*/
static int _SystemView_FormatRecordTime(char *buffer, size_t buffer_size) {
    struct tm Tm;
    time_t    Now;

    if ((buffer == NULL) || (buffer_size == 0u)) {
        return -1;
    }

    Now = time(NULL);
    if (Now == (time_t)-1) {
        return -1;
    }
#if defined(_WIN32)
    if (localtime_s(&Tm, &Now) != 0) {
        return -1;
    }
#else
    if (localtime_r(&Now, &Tm) == NULL) {
        return -1;
    }
#endif
    return (strftime(buffer, buffer_size, "%d %b %Y %H:%M:%S", &Tm) == 0u) ? -1 : 0;
}

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
*       _SystemView_WriteRecordFileHeader()
*
*  Function description
*    Write the textual SVDat container header before binary trace bytes.
*/
int _SystemView_WriteRecordFileHeader(SystemView_State_t *pState, FILE *record_file) {
    char RecordTime[32];
    int  Result;

    if (record_file == NULL) {
        return 0;
    }

    if (_SystemView_FormatRecordTime(RecordTime, sizeof(RecordTime)) != 0) {
        _SystemView_ReportRecordFileError(pState, "header timestamp", 0);
        return -1;
    }

    errno = 0;
    Result = fprintf(record_file,
                     ";\n"
                     "; Version     SEGGER SystemViewer V%u.%02u.%02u\n"
                     "; RecordTime  %s\n"
                     "; Author      CineLogic TraceHub\n"
                     "; Title       TraceHub SystemView Recording\n"
                     "; Description TraceHub SystemView RTT recording, channel %u\n"
                     ";\n"
                     "\n",
                     SYSVIEW_VERSION_MAJOR,
                     SYSVIEW_VERSION_MINOR,
                     SYSVIEW_VERSION_REV,
                     RecordTime,
                     pState != NULL ? pState->Config.channel : 0u);
    if (Result < 0) {
        _SystemView_ReportRecordFileError(pState, "header write", errno);
        return -1;
    }

    return _SystemView_FlushRecordFile(pState, record_file);
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
