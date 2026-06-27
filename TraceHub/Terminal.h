/*********************************************************************
*                                                                    *
*                Copyright (C) 2023 xrTest Inc.                      *
*                      All rights reserved                           *
*                                                                    *
**********************************************************************
----------------------------------------------------------------------
File    : Terminal.h
Purpose : Terminal service for RTT Channel 0 with Telnet protocol
Author  : songwenshuai <songwenshuai@gmail.com>
---------------------------END-OF-HEADER------------------------------
*/

#ifndef TRACEHUB_TERMINAL_H            // Guard against multiple inclusion
#define TRACEHUB_TERMINAL_H

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
*       Types
*
**********************************************************************
*/

/*********************************************************************
*
*       Terminal_Config_t
*
*  Description
*    Configuration structure for Terminal service initialization.
*
*  Fields
*    port          TCP port for Telnet connections (default 19021)
*    channel       RTT channel number (default 0 for Terminal)
*    enabled       Enable/disable flag for the service
*    console_mode  Console mode flag (stdin/stdout instead of TCP socket)
*    log_enabled   Enable logging to file
*    log_prefix    Prefix for log file name (e.g., "terminal")
*    network_queue_size  TCP backlog size in bytes, 0 selects the default
*/
typedef struct {
    unsigned     port;
    unsigned     channel;
    bool         enabled;
    bool         console_mode;
    bool         log_enabled;
    const char  *log_prefix;
    size_t       network_queue_size;
} Terminal_Config_t;

/*********************************************************************
*
*       API functions
*
**********************************************************************
*/

/*********************************************************************
*
*       Terminal_Init()
*
*  Function description
*    Initialize the Terminal service with the specified configuration.
*
*  Parameters
*    config  Pointer to configuration structure
*
*  Return value
*    0   Success
*   -1   Invalid configuration or already initialized
*/
int Terminal_Init(Terminal_Config_t *config);

/*********************************************************************
*
*       Terminal_Start()
*
*  Function description
*    Start the Terminal service threads.
*
*  Return value
*    0   Success
*   -1   Failed to start service
*/
int Terminal_Start(void);

/*********************************************************************
*
*       Terminal_Stop()
*
*  Function description
*    Stop the Terminal service and cleanup resources.
*/
void Terminal_Stop(void);

/*********************************************************************
*
*       Terminal_Status()
*
*  Function description
*    Print current Terminal service status to stdout.
*/
void Terminal_Status(void);

/*********************************************************************
*
*       Terminal_IsEnabled()
*
*  Function description
*    Check if the Terminal service is enabled.
*
*  Return value
*    true   Service is enabled
*    false  Service is disabled
*/
bool Terminal_IsEnabled(void);

/*********************************************************************
*
*       Terminal_HasFatalError()
*
*  Function description
*    Check whether the Terminal service entered a fatal state.
*
*  Return value
*    true   Fatal error occurred
*    false  No fatal error recorded
*/
bool Terminal_HasFatalError(void);

#if defined(__cplusplus)          // Allow usage of this module from C++ files (disable name mangling)
}                /* Make sure we have C-declarations in C++ programs */
#endif

#endif // end of include guard: TRACEHUB_TERMINAL_H Avoid multiple inclusion

/*************************** End of file ****************************/
