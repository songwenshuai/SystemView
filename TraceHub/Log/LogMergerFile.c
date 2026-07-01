/*********************************************************************
*                             CineLogic                              *
*                      RTT Trace and Debug Bridge                    *
**********************************************************************
*                                                                    *
*                    (c) 2023 - 2026 CineLogic                       *
*                                                                    *
*                  Support: wenshuaisong@gmail.com                   *
*                                                                    *
**********************************************************************
*                                                                    *
*       CineLogic TraceHub * RTT trace and debug bridge              *
*                                                                    *
**********************************************************************
*                                                                    *
* All rights reserved.                                               *
*                                                                    *
* CineLogic strongly recommends to not make any changes              *
* to or modify the source code of this software in order to stay     *
* compatible with the SharedMem and RTT data path.                   *
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
* DISCLAIMED. IN NO EVENT SHALL CINELOGIC BE LIABLE FOR              *
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
----------------------------------------------------------------------
File    : LogMergerFile.c
Purpose : Log merger file persistence
---------------------------END-OF-HEADER------------------------------
*/

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include <errno.h>
#include <string.h>

#include "LogMerger_internal.h"

/*********************************************************************
*
*       Public internal functions
*
**********************************************************************
*/

/*********************************************************************
*
*       _Merger_InitLogFileState()
*
*  Function description
*    Initialize source labels and clean-text state for merger-owned files.
*/
void _Merger_InitLogFileState(void) {
    unsigned i;

    _merger_state.source_label[LOG_SOURCE_LINUX] = LOG_SOURCE_DEFAULT_LINUX_LABEL;
    _merger_state.source_label[LOG_SOURCE_RTOS]  = LOG_SOURCE_DEFAULT_RTOS_LABEL;
    for (i = 0; i < LOG_SOURCE_MAX; i++) {
        LOG_TextCleanStateInit(&_merger_state.log_clean_state[i]);
    }
}

/*********************************************************************
*
*       _Merger_OpenLogFile()
*
*  Function description
*    Create the merger-owned log file when file logging is enabled.
*
*  Return value
*    0   Success or logging disabled
*   -1   File creation failed
*/
int _Merger_OpenLogFile(const LogMerger_Config_t *config) {
    if (config == NULL) {
        return -1;
    }
    if (!config->log_enabled || config->log_prefix == NULL) {
        return 0;
    }

    _merger_state.log_file = LOG_CreateTimestampedFile(config->log_prefix);
    if (_merger_state.log_file == NULL) {
        return -1;
    }
    return 0;
}

/*********************************************************************
*
*       _Merger_ReportLogFileErrorLocked()
*
*  Function description
*    Record a merger log file persistence error. Caller holds mutex.
*/
void _Merger_ReportLogFileErrorLocked(const char *operation, int error_code) {
    if (operation == NULL) {
        operation = "operation";
    }
    if (!_merger_state.log_file_error) {
        if (error_code != 0) {
            fprintf(stderr,
                    "[LogMerger] log file %s failed: %s\n",
                    operation, strerror(error_code));
        } else {
            fprintf(stderr,
                    "[LogMerger] log file %s failed\n",
                    operation);
        }
    }
    _merger_state.log_file_error = true;
}

/*********************************************************************
*
*       _Merger_CloseLogFileLocked()
*
*  Function description
*    Close the merger-owned log file. Caller holds mutex.
*/
void _Merger_CloseLogFileLocked(void) {
    FILE *log_file;

    if (_merger_state.log_file == NULL) {
        return;
    }

    log_file = _merger_state.log_file;
    _merger_state.log_file = NULL;
    errno = 0;
    if (fclose(log_file) != 0) {
        _Merger_ReportLogFileErrorLocked("close", errno);
    }
}

/*********************************************************************
*
*       Public functions
*
**********************************************************************
*/

/*********************************************************************
*
*       LogMerger_WriteEntry()
*
*  Function description
*    Persist a merged entry to the merger-owned log file when logging is
*    enabled.
*
*  Return value
*    0   Success or logging disabled.
*   -1   Log file write failed.
*/
int LogMerger_WriteEntry(const LogEntry_t *entry) {
    LogSource_t  source;
    const char  *source_name;
    int          result;
    int          error_code;

    if (entry == NULL) {
        return -1;
    }
    if (!_merger_state.initialized || !_merger_state.mutex_initialized) {
        return -1;
    }

    SYS_MutexLock(&_merger_state.mutex);
    if (_merger_state.log_file == NULL) {
        SYS_MutexUnlock(&_merger_state.mutex);
        return 0;
    }

    source = LogEntry_GetSource(entry);
    if (source >= LOG_SOURCE_MAX) {
        SYS_MutexUnlock(&_merger_state.mutex);
        return -1;
    }

    source_name = _merger_state.source_label[source];
    errno = 0;
    result = LOG_SwimLaneLogToFile(_merger_state.log_file,
                                   LogEntry_GetTimestamp(entry),
                                   source_name,
                                   LogEntry_GetContent(entry),
                                   &_merger_state.log_clean_state[source]);
    error_code = errno;
    if (result != 0) {
        _Merger_ReportLogFileErrorLocked("write", error_code);
        SYS_MutexUnlock(&_merger_state.mutex);
        return -1;
    }

    SYS_MutexUnlock(&_merger_state.mutex);
    return 0;
}

/*********************************************************************
*
*       LogMerger_HasFileError()
*
*  Function description
*    Check whether the merger log file encountered a persistence error.
*
*  Return value
*    true   Persistence error occurred.
*    false  No persistence error recorded.
*/
bool LogMerger_HasFileError(void) {
    bool file_error;

    if (!_merger_state.mutex_initialized) {
        return _merger_state.log_file_error;
    }

    SYS_MutexLock(&_merger_state.mutex);
    file_error = _merger_state.log_file_error;
    SYS_MutexUnlock(&_merger_state.mutex);
    return file_error;
}

/*************************** End of file ****************************/
