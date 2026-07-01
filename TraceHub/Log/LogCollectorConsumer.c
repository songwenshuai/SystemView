/*********************************************************************
*                             CineLogic                              *
*                      RTT Trace and Debug Bridge                    *
**********************************************************************
----------------------------------------------------------------------
File    : LogCollectorConsumer.c
Purpose : LogCollector core recorder consumer ownership
---------------------------END-OF-HEADER------------------------------
*/

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include "LogCollector_internal.h"
#include "CoreLogRecorder.h"

/*********************************************************************
*
*       Public functions
*
**********************************************************************
*/

/*********************************************************************
*
*       LogCollectorState_EnsureConsumersRegistered()
*
*  Function description
*    Ensure LogCollector owns the consumer registrations for both core
*    channels.
*/
int LogCollectorState_EnsureConsumersRegistered(void) {
    int                         result;
    LogCollector_SourceState_t *linux_source;
    LogCollector_SourceState_t *rtos_source;
    bool                        linux_registered_here;

    linux_source = &_collector_state.sources[LOG_SOURCE_LINUX];
    rtos_source = &_collector_state.sources[LOG_SOURCE_RTOS];

    linux_registered_here = false;
    if (!linux_source->consumer_registered) {
        result = CoreLogRecorder_RegisterConsumer(linux_source->channel);
        if (result != 0) {
            return -1;
        }
        linux_source->consumer_registered = true;
        linux_registered_here = true;
    }

    if (!rtos_source->consumer_registered) {
        result = CoreLogRecorder_RegisterConsumer(rtos_source->channel);
        if (result != 0) {
            if (linux_registered_here) {
                CoreLogRecorder_UnregisterConsumer(linux_source->channel);
                linux_source->consumer_registered = false;
            }
            return -1;
        }
        rtos_source->consumer_registered = true;
    }

    return 0;
}

/*********************************************************************
*
*       LogCollectorState_UnregisterConsumers()
*
*  Function description
*    Release any core recorder consumer registrations owned by LogCollector.
*/
void LogCollectorState_UnregisterConsumers(void) {
    LogCollector_SourceState_t *linux_source;
    LogCollector_SourceState_t *rtos_source;

    linux_source = &_collector_state.sources[LOG_SOURCE_LINUX];
    rtos_source = &_collector_state.sources[LOG_SOURCE_RTOS];

    if (rtos_source->consumer_registered) {
        CoreLogRecorder_UnregisterConsumer(rtos_source->channel);
        rtos_source->consumer_registered = false;
    }
    if (linux_source->consumer_registered) {
        CoreLogRecorder_UnregisterConsumer(linux_source->channel);
        linux_source->consumer_registered = false;
    }
}

/*********************************************************************
*
*       LogCollectorState_StopCollectionWithResult()
*
*  Function description
*    Release collector consumers and return a terminal collection result.
*/
int LogCollectorState_StopCollectionWithResult(int result) {
    if (result == LOG_COLLECT_RESULT_INVALID_RTT ||
        result == LOG_COLLECT_RESULT_PENDING_OVERFLOW) {
        LogCollectorState_SetFatalErrorResult(result);
    }
    LogCollectorState_UnregisterConsumers();
    return result;
}

/*************************** End of file ****************************/
