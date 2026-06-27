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
File    : SystemView.h
Purpose : SystemView/TRACE service for a configured RTT channel
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
*    pMessage     Output buffer.
*    MessageSize  Output buffer size in bytes.
*
*  Return value
*    true   Message was built.
*    false  Invalid buffer contract.
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
*    pMessage     Input message.
*    MessageSize  Input message size in bytes.
*
*  Return value
*    true   Prefix is valid.
*    false  Prefix or buffer contract is invalid.
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
*/
typedef struct {
    unsigned     port;
    unsigned     channel;
    bool         enabled;
    bool         network_enabled;
    bool         record_enabled;
    const char  *record_prefix;
} SystemView_Config_t;

/*********************************************************************
*
*       API functions
*
**********************************************************************
*/

int  SystemView_Init         (SystemView_Config_t *config);
int  SystemView_Start        (void);
void SystemView_Stop         (void);
void SystemView_Status       (void);
bool SystemView_IsEnabled    (void);
bool SystemView_HasFatalError(void);

#if defined(__cplusplus)          // Allow usage of this module from C++ files (disable name mangling)
}                /* Make sure we have C-declarations in C++ programs */
#endif

#endif // end of include guard: TRACEHUB_SYSTEMVIEW_H Avoid multiple inclusion

/*************************** End of file ****************************/
