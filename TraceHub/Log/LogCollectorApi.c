/*********************************************************************
*                             CineLogic                              *
*                      RTT Trace and Debug Bridge                    *
**********************************************************************
----------------------------------------------------------------------
File    : LogCollectorApi.c
Purpose : LogCollector public lifecycle wrappers
---------------------------END-OF-HEADER------------------------------
*/

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include "LogCollector_internal.h"

/*********************************************************************
*
*       Public functions
*
**********************************************************************
*/

/*********************************************************************
*
*       LogCollector_Init()
*
*  Function description
*    Initialize the log collector with the specified configuration.
*
*  Parameters
*    config  Pointer to configuration structure.
*
*  Return value
*    0   Success.
*   -1   Invalid configuration.
*/
int LogCollector_Init(LogCollector_Config_t *config) {
    LogCollector_State_t *state;

    if (config == NULL) {
        return -1;
    }
    if (LogCollectorState_IsRunning()) {
        return -1;
    }

    state = LogCollectorState_Get();
    if (state->initialized) {
        LogCollector_Cleanup();
    }

    return LogCollectorState_Init(config);
}

/*********************************************************************
*
*       LogCollector_Cleanup()
*
*  Function description
*    Cleanup log collector resources.
*/
void LogCollector_Cleanup(void) {
    LogCollector_State_t *state;
    unsigned              i;

    state = LogCollectorState_Get();
    if (LogCollectorState_IsRunning() || LogCollectorState_AnyThreadStarted()) {
        LogCollector_Stop();
    }
    for (i = 0u; i < LOG_SOURCE_MAX; i++) {
        LogCollectorSource_ReportUnflushedUntimedLines(&state->sources[i]);
    }
    LogCollectorState_Destroy();
}

/*********************************************************************
*
*       LogCollector_IsRunning()
*
*  Function description
*    Check whether log collection threads are running.
*
*  Return value
*    true   Collector is running.
*    false  Collector is stopped.
*/
bool LogCollector_IsRunning(void) {
    return LogCollectorState_IsRunning();
}

/*********************************************************************
*
*       LogCollector_HasFatalError()
*
*  Function description
*    Check whether the collector has entered a fatal error state.
*
*  Return value
*    true   Fatal error occurred.
*    false  No fatal error recorded.
*/
bool LogCollector_HasFatalError(void) {
    return LogCollectorState_HasFatalError();
}

/*************************** End of file ****************************/
