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

Purpose : Host-side RTT configuration for the SystemView public API
          unit tests.

          The production SystemView source is linked against RTT API
          stubs supplied by the test driver.  These definitions provide
          the RTT constants required by SEGGER_SYSVIEW.c and define the
          shared-memory control-block address expected by the stubs.
*/

#ifndef SEGGER_RTT_CONF_H
#define SEGGER_RTT_CONF_H

/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/

//
// Keep the unit-test channel count small, but allow the default
// terminal channel, the SystemView channel, and one additional buffer.
//
#define SEGGER_RTT_MAX_NUM_UP_BUFFERS          4
#define SEGGER_RTT_MAX_NUM_DOWN_BUFFERS        4

//
// Default RTT channel 0 sizes used by the RTT configuration defaults.
// The SystemView test does not exercise these buffers directly, but the
// values keep the included RTT headers self-contained.
//
#define BUFFER_SIZE_UP                         128
#define BUFFER_SIZE_DOWN                       16

//
// The SystemView test verifies that every RTT call made by
// SEGGER_SYSVIEW.c uses this explicit shared-memory control-block base.
//
#define SEGGER_RTT_CB_ADDRESS                  (0x12340000u)

//
// Host-side tests run single-threaded and do not need target locks.
//
#define SEGGER_RTT_LOCK()
#define SEGGER_RTT_UNLOCK()

#endif  // SEGGER_RTT_CONF_H

/*************************** End of file ****************************/
