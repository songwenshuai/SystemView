/*********************************************************************
*                   (c) SEGGER Microcontroller GmbH                  *
*                        The Embedded Experts                        *
*                           www.segger.com                           *
**********************************************************************
*                                                                    *
*         SEGGER SystemView  * Real-time application analysis        *
*              https://github.com/SEGGERMicro/SystemView             *
*                                                                    *
**********************************************************************

---------------------------END-OF-HEADER------------------------------

Purpose : Host-side SystemView configuration for the public API unit
          tests.

          The settings below force host-safe timestamp and interrupt
          access, configure deterministic RTT channel parameters, and
          route event-recorded notifications back into the test driver.
*/

#ifndef SEGGER_SYSVIEW_CONF_H
#define SEGGER_SYSVIEW_CONF_H

/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/

//
// Use the generic core path so tests never access target CPU registers.
//
#define SEGGER_SYSVIEW_CORE                    0

//
// The test RTT stubs validate that SystemView passes these explicit
// shared-memory RTT addresses to every RTT API call.
//
#define SEGGER_SYSVIEW_RTT_CB_ADDRESS          (0x12340000u)
#define SEGGER_SYSVIEW_RTT_NAME_ADDRESS        (0x12340100u)

//
// Small deterministic packet and buffer sizes keep the host test fast
// while still exercising packet encoding and string limiting paths.
//
#define SEGGER_SYSVIEW_RTT_BUFFER_SIZE         256
#define SEGGER_SYSVIEW_MAX_STRING_LEN          64
#define SEGGER_SYSVIEW_MAX_ARGUMENTS           16
#define SEGGER_SYSVIEW_USE_STATIC_BUFFER       1
#define SEGGER_SYSVIEW_CPU_CACHE_LINE_SIZE     0

//
// Host-side tests are single-threaded.  Event notifications are routed
// to a test hook so the test can verify that packet writes were reported.
//
#define SEGGER_SYSVIEW_LOCK()
#define SEGGER_SYSVIEW_UNLOCK()
#define SEGGER_SYSVIEW_ON_EVENT_RECORDED(NumBytes) SEGGER_SYSVIEW_X_OnEventRecorded(NumBytes)

#endif  // SEGGER_SYSVIEW_CONF_H

/*************************** End of file ****************************/
