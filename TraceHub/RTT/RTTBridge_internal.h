/*********************************************************************
*                             CineLogic                              *
*                      RTT Trace and Debug Bridge                    *
**********************************************************************
----------------------------------------------------------------------
File    : RTTBridge_internal.h
Purpose : Internal RTT bridge state and function contracts
---------------------------END-OF-HEADER------------------------------
*/

#ifndef TRACEHUB_RTTBRIDGE_INTERNAL_H
#define TRACEHUB_RTTBRIDGE_INTERNAL_H

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include <stddef.h>
#include <stdint.h>

#include "RTTBridge.h"
#include "SYS.h"

#if defined(__cplusplus)
extern "C" {
#endif

/*********************************************************************
*
*       Shared state
*
**********************************************************************
*/

extern RTTBridge_State_t _bridge_state;
extern SYS_Mutex         _bridge_lock;

/*********************************************************************
*
*       State functions
*
**********************************************************************
*/

void RTTBridgeState_Lock  (void);
void RTTBridgeState_Unlock(void);

/*********************************************************************
*
*       Region functions
*
**********************************************************************
*/

int  RTTBridgeRegion_SetupMemoryMapping  (const char *device_path, uint64_t backend_address, size_t map_size);
void RTTBridgeRegion_CleanupMemoryMapping(void);
int  RTTBridgeRegion_FindValid           (uintptr_t *address, size_t *region_size);

#if defined(__cplusplus)
}
#endif

#endif /* TRACEHUB_RTTBRIDGE_INTERNAL_H */

/*************************** End of file ****************************/
