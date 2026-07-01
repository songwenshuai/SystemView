/*********************************************************************
*                             CineLogic                              *
*                      RTT Trace and Debug Bridge                    *
**********************************************************************
----------------------------------------------------------------------
File    : RTTBridgeState.c
Purpose : RTT bridge shared state and statistics
---------------------------END-OF-HEADER------------------------------
*/

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include <inttypes.h>
#include <stdio.h>

#include "RTTBridge_internal.h"

/*********************************************************************
*
*       Shared state
*
**********************************************************************
*/

RTTBridge_State_t _bridge_state;
SYS_Mutex         _bridge_lock = SYS_MUTEX_INITIALIZER;

/*********************************************************************
*
*       Internal functions
*
**********************************************************************
*/

/*********************************************************************
*
*       RTTBridgeState_Lock()
*
*  Function description
*    Lock bridge module state.
*/
void RTTBridgeState_Lock(void) {
    SYS_MutexLock(&_bridge_lock);
}

/*********************************************************************
*
*       RTTBridgeState_Unlock()
*
*  Function description
*    Unlock bridge state after RTTBridgeState_Lock() succeeds.
*/
void RTTBridgeState_Unlock(void) {
    SYS_MutexUnlock(&_bridge_lock);
}

/*********************************************************************
*
*       Public functions
*
**********************************************************************
*/

/*********************************************************************
*
*       RTTBridge_GetState()
*
*  Function description
*    Get pointer to the bridge state structure.
*
*  Return value
*    Read-only pointer to RTTBridge_State_t structure.
*/
const RTTBridge_State_t *RTTBridge_GetState(void) {
    return &_bridge_state;
}

/*********************************************************************
*
*       RTTBridge_IsRunning()
*
*  Function description
*    Check if the bridge is currently running.
*
*  Return value
*    true   Bridge is running.
*    false  Bridge is not running.
*/
bool RTTBridge_IsRunning(void) {
    bool running;

    RTTBridgeState_Lock();
    running = _bridge_state.initialized && _bridge_state.running;
    RTTBridgeState_Unlock();
    return running;
}

/*********************************************************************
*
*       RTTBridge_SetRunning()
*
*  Function description
*    Set the bridge running state.
*
*  Parameters
*    running  New running state.
*/
void RTTBridge_SetRunning(bool running) {
    RTTBridgeState_Lock();
    if (!_bridge_state.initialized) {
        RTTBridgeState_Unlock();
        return;
    }

    _bridge_state.running = running;
    RTTBridgeState_Unlock();
}

/*********************************************************************
*
*       RTTBridge_IncrementPolls()
*
*  Function description
*    Thread-safe increment of polls counter.
*/
void RTTBridge_IncrementPolls(void) {
    RTTBridgeState_Lock();
    if (!_bridge_state.initialized) {
        RTTBridgeState_Unlock();
        return;
    }

    _bridge_state.polls_count++;
    RTTBridgeState_Unlock();
}

/*********************************************************************
*
*       RTTBridge_IncrementErrors()
*
*  Function description
*    Thread-safe increment of errors counter.
*/
void RTTBridge_IncrementErrors(void) {
    RTTBridgeState_Lock();
    if (!_bridge_state.initialized) {
        RTTBridgeState_Unlock();
        return;
    }

    _bridge_state.errors_count++;
    RTTBridgeState_Unlock();
}

/*********************************************************************
*
*       RTTBridge_Status()
*
*  Function description
*    Print current bridge status to stdout.
*/
void RTTBridge_Status(void) {
    RTTBridgeState_Lock();
    if (!_bridge_state.initialized) {
        RTTBridgeState_Unlock();
        printf("RTT Bridge Status: not initialized\n");
        return;
    }

    printf("RTT Bridge Status:\n");
    printf("  RTT CB Address: 0x%" PRIxPTR "\n", _bridge_state.rtt_cb_address);
    printf("  RTT Region Size: %zu\n", _bridge_state.rtt_region_size);
    printf("  Running:        %s\n", _bridge_state.running ? "yes" : "no");
    printf("  Polls Count:    %u\n", _bridge_state.polls_count);
    printf("  Errors Count:   %u\n", _bridge_state.errors_count);
    RTTBridgeState_Unlock();
}

/*************************** End of file ****************************/
