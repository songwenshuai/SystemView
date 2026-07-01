/*********************************************************************
*                             CineLogic                              *
*                      RTT Trace and Debug Bridge                    *
**********************************************************************
----------------------------------------------------------------------
File    : RTTBridgeApi.c
Purpose : RTT bridge initialization and cleanup public API
---------------------------END-OF-HEADER------------------------------
*/

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include <inttypes.h>
#include <limits.h>
#include <string.h>

#include "RTTBridge_internal.h"
#include "Log.h"
#include "SEGGER_RTT.h"

/*********************************************************************
*
*       Defines
*
**********************************************************************
*/

#ifndef RTT_CB_SEARCH_RETRY_INTERVAL_MS
  #define RTT_CB_SEARCH_RETRY_INTERVAL_MS   1000    /* Retry interval in ms */
#endif

#ifndef RTT_CB_SEARCH_MAX_RETRIES
  #define RTT_CB_SEARCH_MAX_RETRIES         0       /* 0 = infinite retries */
#endif

/*********************************************************************
*
*       Public functions
*
**********************************************************************
*/

/*********************************************************************
*
*       RTTBridge_Init()
*
*  Function description
*    Initialize the RTT bridge with the specified configuration.
*    Sets up memory mapping and locates RTT Control Block.
*
*  Parameters
*    config  Pointer to configuration structure.
*
*  Return value
*    0   Success.
*   -1   Failed to setup memory mapping.
*   -2   Failed to find RTT Control Block.
*   -3   Initialization aborted by run flag.
*/
int RTTBridge_Init(RTTBridge_Config_t *config) {
    int       ret;
    uintptr_t address;
    size_t    region_size = 0u;
    unsigned  retry_count = 0u;
    unsigned  elapsed_ms = 0u;
    unsigned  sleep_ms;

    if ((config == NULL) ||
        ((config->rtt_region_size != 0u) && (config->rtt_region_size < SEGGER_RTT__CB_OFF_A_UP))) {
        return -1;
    }

    RTTBridgeState_Lock();
    if (_bridge_state.initialized || _bridge_state.memory_initialized) {
        RTTBridgeState_Unlock();
        return -1;
    }
    memset(&_bridge_state, 0, sizeof(_bridge_state));
    memcpy(&_bridge_state.config, config, sizeof(RTTBridge_Config_t));
    RTTBridgeState_Unlock();

    Log_Print("Initializing RTT Bridge\n");
    Log_Print("RTT region: addr=0x%016" PRIx64 ", size=0x%lx (%zu bytes)\n",
              config->rtt_address,
              (unsigned long)config->rtt_region_size,
              config->rtt_region_size);

    ret = RTTBridgeRegion_SetupMemoryMapping(config->device_path,
                                             config->rtt_address,
                                             config->rtt_region_size);
    if (ret != 0) {
        Log_Error("Failed to setup memory mapping\n");
        RTTBridge_Cleanup();
        return -1;
    }

    Log_Print("Searching for RTT control block (will retry if not found)...\n");

    while (1) {
        if ((config->run_flag != NULL) && (*config->run_flag == 0)) {
            Log_Warn("RTT control block search aborted by run flag\n");
            RTTBridge_Cleanup();
            return -3;
        }

        ret = RTTBridgeRegion_FindValid(&address, &region_size);

        if (ret == -2) {
            Log_Error("RTT control block search stopped because the mapped range is invalid\n");
            RTTBridge_Cleanup();
            return -1;
        }

        if (ret == 0) {
            RTTBridgeState_Lock();
            _bridge_state.rtt_cb_address = address;
            _bridge_state.rtt_region_size = region_size;
            _bridge_state.initialized = true;
            RTTBridgeState_Unlock();
            Log_Print("Found RTT control block at local address 0x%" PRIxPTR "\n",
                      address);
            return 0;
        }

        retry_count++;

        if (RTT_CB_SEARCH_MAX_RETRIES > 0 && retry_count >= RTT_CB_SEARCH_MAX_RETRIES) {
            Log_Error("Failed to find RTT control block after %u retries\n", retry_count);
            RTTBridge_Cleanup();
            return -2;
        }
        if ((config->rtt_search_timeout_ms > 0u) &&
            (elapsed_ms >= config->rtt_search_timeout_ms)) {
            Log_Error("Failed to find RTT control block after %u ms\n", elapsed_ms);
            RTTBridge_Cleanup();
            return -2;
        }

        if (retry_count == 1u) {
            Log_Print("RTT control block not found, waiting for target initialization...\n");
        } else if ((retry_count % 10u) == 0u) {
            Log_Print("Still waiting for RTT control block (retry %u)...\n", retry_count);
        }

        sleep_ms = RTT_CB_SEARCH_RETRY_INTERVAL_MS;
        if (config->rtt_search_timeout_ms > 0u) {
            unsigned remaining_ms;

            remaining_ms = config->rtt_search_timeout_ms - elapsed_ms;
            if (sleep_ms > remaining_ms) {
                sleep_ms = remaining_ms;
            }
        }
        SYS_Sleep(sleep_ms);
        if (sleep_ms > (UINT_MAX - elapsed_ms)) {
            elapsed_ms = UINT_MAX;
        } else {
            elapsed_ms += sleep_ms;
        }
    }
}

/*********************************************************************
*
*       RTTBridge_Cleanup()
*
*  Function description
*    Cleanup RTT bridge resources and unmap RTT memory.
*/
void RTTBridge_Cleanup(void) {
    RTTBridgeState_Lock();
    _bridge_state.running = false;
    _bridge_state.initialized = false;
    RTTBridgeState_Unlock();

    RTTBridgeRegion_CleanupMemoryMapping();

    RTTBridgeState_Lock();
    memset(&_bridge_state, 0, sizeof(_bridge_state));
    RTTBridgeState_Unlock();
}

/*************************** End of file ****************************/
