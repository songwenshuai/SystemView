/*********************************************************************
*                                                                    *
*                Copyright (C) 2023 xrTest Inc.                      *
*                      All rights reserved                           *
*                                                                    *
**********************************************************************
----------------------------------------------------------------------
File    : SystemView.h
Purpose : SystemView/TRACE service for a configured RTT channel
Author  : songwenshuai <songwenshuai@gmail.com>
---------------------------END-OF-HEADER------------------------------
*/

#ifndef TRACEHUB_SYSTEMVIEW_H            // Guard against multiple inclusion
#define TRACEHUB_SYSTEMVIEW_H

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

#if defined(__cplusplus)         // Allow usage of this module from C++ files (disable name mangling)
extern "C" {     /* Make sure we have C-declarations in C++ programs */
#endif

/*********************************************************************
*
*       Defines, fixed
*
**********************************************************************
*/

/*********************************************************************
*
*       SYSVIEW_HELLO_SIZE
*  Size of SystemView handshake Hello message (32 bytes).
*
*/
#define SYSVIEW_HELLO_SIZE          32

/*********************************************************************
*
*       SYSVIEW_HELLO_PREFIX
*  SystemView handshake Hello prefix.
*
*/
#define SYSVIEW_HELLO_PREFIX        "SEGGER SystemView"
#define SYSVIEW_HELLO_PREFIX_SIZE   (sizeof(SYSVIEW_HELLO_PREFIX) - 1u)

/*********************************************************************
*
*       SYSVIEW_VERSION_MAJOR
*  SystemView protocol major version number.
*
*/
#define SYSVIEW_VERSION_MAJOR       3

/*********************************************************************
*
*       SYSVIEW_VERSION_MINOR
*  SystemView protocol minor version number.
*
*/
#define SYSVIEW_VERSION_MINOR       60

/*********************************************************************
*
*       SYSVIEW_VERSION_REV
*  SystemView protocol revision number.
*
*/
#define SYSVIEW_VERSION_REV         0

#if (SYSVIEW_VERSION_MAJOR < 0) || (SYSVIEW_VERSION_MAJOR > 9)
#error "SYSVIEW_VERSION_MAJOR must fit one decimal digit"
#endif

#if (SYSVIEW_VERSION_MINOR < 0) || (SYSVIEW_VERSION_MINOR > 99)
#error "SYSVIEW_VERSION_MINOR must fit two decimal digits"
#endif

#if (SYSVIEW_VERSION_REV < 0) || (SYSVIEW_VERSION_REV > 99)
#error "SYSVIEW_VERSION_REV must fit two decimal digits"
#endif

#if defined(__cplusplus)
static_assert((SYSVIEW_HELLO_PREFIX_SIZE + 9u) <= SYSVIEW_HELLO_SIZE,
              "SystemView Hello message format exceeds 32 bytes");
#else
_Static_assert((SYSVIEW_HELLO_PREFIX_SIZE + 9u) <= SYSVIEW_HELLO_SIZE,
               "SystemView Hello message format exceeds 32 bytes");
#endif

/*********************************************************************
*
*       Inline helper functions
*
**********************************************************************
*/

/*********************************************************************
*
*       SystemView_BuildHelloMessage()
*
*  Function description
*    Build the 32-byte SystemView Hello message from the shared protocol
*    version constants.
*
*  Parameters
*    pMessage     Output buffer
*    MessageSize  Output buffer size in bytes
*
*  Return value
*    true   Message was built
*    false  Invalid buffer contract
*/
static inline bool SystemView_BuildHelloMessage(unsigned char *pMessage, size_t MessageSize) {
    static const char acPrefix[] = SYSVIEW_HELLO_PREFIX;
    size_t            i;

    if ((pMessage == NULL) || (MessageSize != SYSVIEW_HELLO_SIZE)) {
        return false;
    }

    for (i = 0u; i < MessageSize; i++) {
        pMessage[i] = 0u;
    }
    for (i = 0u; i < SYSVIEW_HELLO_PREFIX_SIZE; i++) {
        pMessage[i] = (unsigned char)acPrefix[i];
    }

    pMessage[SYSVIEW_HELLO_PREFIX_SIZE]      = ' ';
    pMessage[SYSVIEW_HELLO_PREFIX_SIZE + 1u] = 'V';
    pMessage[SYSVIEW_HELLO_PREFIX_SIZE + 2u] = (unsigned char)('0' + SYSVIEW_VERSION_MAJOR);
    pMessage[SYSVIEW_HELLO_PREFIX_SIZE + 3u] = '.';
    pMessage[SYSVIEW_HELLO_PREFIX_SIZE + 4u] = (unsigned char)('0' + (SYSVIEW_VERSION_MINOR / 10));
    pMessage[SYSVIEW_HELLO_PREFIX_SIZE + 5u] = (unsigned char)('0' + (SYSVIEW_VERSION_MINOR % 10));
    pMessage[SYSVIEW_HELLO_PREFIX_SIZE + 6u] = '.';
    pMessage[SYSVIEW_HELLO_PREFIX_SIZE + 7u] = (unsigned char)('0' + (SYSVIEW_VERSION_REV / 10));
    pMessage[SYSVIEW_HELLO_PREFIX_SIZE + 8u] = (unsigned char)('0' + (SYSVIEW_VERSION_REV % 10));
    return true;
}

/*********************************************************************
*
*       SystemView_HelloHasValidPrefix()
*
*  Function description
*    Check whether a 32-byte SystemView Hello message starts with the
*    required SEGGER SystemView prefix.
*
*  Parameters
*    pMessage     Input message
*    MessageSize  Input message size in bytes
*
*  Return value
*    true   Prefix is valid
*    false  Prefix or buffer contract is invalid
*/
static inline bool SystemView_HelloHasValidPrefix(const unsigned char *pMessage, size_t MessageSize) {
    static const char acPrefix[] = SYSVIEW_HELLO_PREFIX;
    size_t            i;

    if ((pMessage == NULL) || (MessageSize != SYSVIEW_HELLO_SIZE)) {
        return false;
    }

    for (i = 0u; i < SYSVIEW_HELLO_PREFIX_SIZE; i++) {
        if (pMessage[i] != (unsigned char)acPrefix[i]) {
            return false;
        }
    }
    return true;
}

/*********************************************************************
*
*       Types
*
**********************************************************************
*/

/*********************************************************************
*
*       SystemView_Config_t
*
*  Description
*    Configuration structure for SystemView service initialization.
*
*  Fields
*    port            TCP port for SystemView connections (default 19111)
*    channel         RTT channel number (default 2 for SystemView)
*    enabled         Enable/disable flag for the service
*    network_enabled Enable/disable network service (true: network service, false: local recording only)
*    record_enabled  Enable recording to file
*    record_prefix   Prefix for record file name (e.g., "sysview")
*    network_queue_size  TCP backlog size in bytes, 0 selects the default
*/
typedef struct {
    unsigned     port;
    unsigned     channel;
    bool         enabled;
    bool         network_enabled;
    bool         record_enabled;
    const char  *record_prefix;
    size_t       network_queue_size;
} SystemView_Config_t;

/*********************************************************************
*
*       API functions
*
**********************************************************************
*/

/*********************************************************************
*
*       SystemView_Init()
*
*  Function description
*    Initialize the SystemView service with the specified configuration.
*
*  Parameters
*    config  Pointer to configuration structure
*
*  Return value
*    0   Success
*   -1   Invalid configuration or already initialized
*/
int SystemView_Init(SystemView_Config_t *config);

/*********************************************************************
*
*       SystemView_Start()
*
*  Function description
*    Start the recording thread and optional network service thread.
*
*  Return value
*    0   Success
*   -1   Failed to start service
*/
int SystemView_Start(void);

/*********************************************************************
*
*       SystemView_Stop()
*
*  Function description
*    Stop the SystemView service and cleanup resources.
*/
void SystemView_Stop(void);

/*********************************************************************
*
*       SystemView_Status()
*
*  Function description
*    Print current SystemView service status to stdout.
*/
void SystemView_Status(void);

/*********************************************************************
*
*       SystemView_IsEnabled()
*
*  Function description
*    Check if the SystemView service is enabled.
*
*  Return value
*    true   Service is enabled
*    false  Service is disabled
*/
bool SystemView_IsEnabled(void);

/*********************************************************************
*
*       SystemView_HasFatalError()
*
*  Function description
*    Check whether the SystemView service entered a fatal state.
*
*  Return value
*    true   Fatal error occurred
*    false  No fatal error recorded
*/
bool SystemView_HasFatalError(void);

#if defined(__cplusplus)          // Allow usage of this module from C++ files (disable name mangling)
}                /* Make sure we have C-declarations in C++ programs */
#endif

#endif // end of include guard: TRACEHUB_SYSTEMVIEW_H Avoid multiple inclusion

/*************************** End of file ****************************/
