/*********************************************************************
*                             CineLogic                              *
*                      RTT Trace and Debug Bridge                    *
**********************************************************************
----------------------------------------------------------------------
File    : LogCollectorError.c
Purpose : LogCollector fatal state and diagnostics
---------------------------END-OF-HEADER------------------------------
*/

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include <stdio.h>

#include "LogCollector_internal.h"

/*********************************************************************
*
*       Static functions
*
**********************************************************************
*/

/*********************************************************************
*
*       _Collector_NormalizeFatalResult()
*
*  Function description
*    Convert any terminal collector failure into the public poll result.
*/
static int _Collector_NormalizeFatalResult(int result) {
    if (result == LOG_COLLECT_RESULT_STOP ||
        result == LOG_COLLECT_RESULT_PENDING_OVERFLOW ||
        result == LOG_COLLECT_RESULT_INVALID_RTT) {
        return result;
    }
    return LOG_COLLECT_RESULT_INVALID_RTT;
}

/*********************************************************************
*
*       Public functions
*
**********************************************************************
*/

/*********************************************************************
*
*       LogCollectorState_HasFatalError()
*
*  Function description
*    Read the collector fatal state through the module lock.
*/
bool LogCollectorState_HasFatalError(void) {
    bool fatal_error;

    if (!_collector_state.lock_initialized) {
        return _collector_state.fatal_error;
    }

    SYS_MutexLock(&_collector_state.lock);
    fatal_error = _collector_state.fatal_error;
    SYS_MutexUnlock(&_collector_state.lock);
    return fatal_error;
}

/*********************************************************************
*
*       LogCollectorState_GetFatalResult()
*
*  Function description
*    Read the fatal result through the module lock.
*/
int LogCollectorState_GetFatalResult(void) {
    int result;

    if (!_collector_state.lock_initialized) {
        if (!_collector_state.fatal_error) {
            return 0;
        }
        return _Collector_NormalizeFatalResult(_collector_state.fatal_result);
    }

    SYS_MutexLock(&_collector_state.lock);
    if (_collector_state.fatal_error) {
        result = _Collector_NormalizeFatalResult(_collector_state.fatal_result);
    } else {
        result = 0;
    }
    SYS_MutexUnlock(&_collector_state.lock);
    return result;
}

/*********************************************************************
*
*       LogCollectorState_SetFatalErrorResult()
*
*  Function description
*    Mark the collector as failed with a stable terminal result.
*/
void LogCollectorState_SetFatalErrorResult(int result) {
    result = _Collector_NormalizeFatalResult(result);

    if (!_collector_state.lock_initialized) {
        if (!_collector_state.fatal_error) {
            _collector_state.fatal_result = result;
        }
        _collector_state.fatal_error = true;
        _collector_state.running = false;
        return;
    }

    SYS_MutexLock(&_collector_state.lock);
    if (!_collector_state.fatal_error) {
        _collector_state.fatal_result = result;
    }
    _collector_state.fatal_error = true;
    _collector_state.running = false;
    SYS_MutexUnlock(&_collector_state.lock);
}

/*********************************************************************
*
*       LogCollectorState_SetFatalError()
*
*  Function description
*    Mark the collector as failed and stop background collection.
*/
void LogCollectorState_SetFatalError(void) {
    LogCollectorState_SetFatalErrorResult(LOG_COLLECT_RESULT_INVALID_RTT);
}

/*********************************************************************
*
*       LogCollectorState_EnterFatalStateWithResult()
*
*  Function description
*    Mark collector as failed and report whether this is the first fatal
*    transition.
*/
bool LogCollectorState_EnterFatalStateWithResult(int result) {
    bool should_report;

    result = _Collector_NormalizeFatalResult(result);

    if (!_collector_state.lock_initialized) {
        should_report = !_collector_state.fatal_error;
        if (should_report) {
            _collector_state.fatal_result = result;
        }
        _collector_state.fatal_error = true;
        _collector_state.running = false;
        return should_report;
    }

    SYS_MutexLock(&_collector_state.lock);
    should_report = !_collector_state.fatal_error;
    if (should_report) {
        _collector_state.fatal_result = result;
    }
    _collector_state.fatal_error = true;
    _collector_state.running = false;
    SYS_MutexUnlock(&_collector_state.lock);
    return should_report;
}

/*********************************************************************
*
*       LogCollectorState_EnterFatalState()
*
*  Function description
*    Mark collector as failed with the default fatal result.
*/
bool LogCollectorState_EnterFatalState(void) {
    return LogCollectorState_EnterFatalStateWithResult(LOG_COLLECT_RESULT_INVALID_RTT);
}

/*********************************************************************
*
*       LogCollectorState_ReportRTTError()
*
*  Function description
*    Report an unrecoverable collector RTT or recorder error.
*/
void LogCollectorState_ReportRTTError(LogSource_t source, const char *operation) {
    bool should_report;

    if (operation == NULL) {
        operation = "access";
    }

    should_report = LogCollectorState_EnterFatalState();
    if (should_report) {
        fprintf(stderr,
                "[LogCollector] %s: fatal %s failure\n",
                LogCollectorState_GetSourceName(source),
                operation);
    }
}

/*********************************************************************
*
*       LogCollectorState_ReportCallbackStop()
*
*  Function description
*    Report an unrecoverable callback stop request from background
*    collection.
*/
void LogCollectorState_ReportCallbackStop(LogSource_t source) {
    if (LogCollectorState_EnterFatalStateWithResult(LOG_COLLECT_RESULT_STOP)) {
        fprintf(stderr,
                "[LogCollector] %s: output callback requested stop\n",
                LogCollectorState_GetSourceName(source));
    }
}

/*********************************************************************
*
*       LogCollectorState_ReportFlushError()
*
*  Function description
*    Report a pending-line flush failure during shutdown.
*/
void LogCollectorState_ReportFlushError(int result) {
    const char *reason;

    if (result >= 0 || result == LOG_COLLECT_RESULT_PENDING_OVERFLOW) {
        return;
    }

    if (result == LOG_COLLECT_RESULT_STOP) {
        reason = "output callback requested stop while flushing pending lines";
    } else if (result == LOG_COLLECT_RESULT_INVALID_RTT) {
        reason = "invalid pending line state";
    } else {
        reason = "unexpected pending line flush failure";
    }

    if (LogCollectorState_EnterFatalStateWithResult(result)) {
        fprintf(stderr, "[LogCollector] fatal shutdown flush failure: %s\n", reason);
    }
}

/*********************************************************************
*
*       LogCollectorState_RecordPendingOverflow()
*
*  Function description
*    Report that leading untimestamped lines exceeded the pending buffer.
*/
void LogCollectorState_RecordPendingOverflow(LogSource_t source, size_t limit) {
    fprintf(stderr,
            "[LogCollector] %s: untimestamped startup log exceeds %zu pending bytes; stopping to avoid reordering\n",
            LogCollectorState_GetSourceName(source),
            limit);
    LogCollectorState_SetFatalErrorResult(LOG_COLLECT_RESULT_PENDING_OVERFLOW);
}

/*************************** End of file ****************************/
