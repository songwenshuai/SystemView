/*********************************************************************
*                                                                    *
*                Copyright (C) 2023 xrTest Inc.                      *
*                      All rights reserved                           *
*                                                                    *
**********************************************************************
----------------------------------------------------------------------
File    : Socket.h
Purpose : TCP socket abstraction layer for Linux platform
Author  : songwenshuai <songwenshuai@gmail.com>
---------------------------END-OF-HEADER------------------------------
*/

#ifndef TRACEHUB_SOCKET_H            // Guard against multiple inclusion
#define TRACEHUB_SOCKET_H

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include <stdbool.h>
#include <stdint.h>         // Type definitions: uint8_t, uint16_t, uint32_t, int8_t, int16_t, int32_t
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>         // For va_list.
#include <stddef.h>         // for size_t
#include <sys/socket.h>     // For SOL_SOCKET and SO_BROADCAST

#if defined(__cplusplus)         // Allow usage of this module from C++ files (disable name mangling)
extern "C" {     /* Make sure we have C-declarations in C++ programs */
#endif

/*********************************************************************
*
*       Defines
*
**********************************************************************
*/

//
// Linux/POSIX socket constants.
//
#define _SOL_SOCKET         SOL_SOCKET
#define _SO_BROADCAST       SO_BROADCAST

//
// Invalid socket handle value.
//
#define SYS_SOCKET_INVALID_HANDLE      (-1)

//
// POSIX-compatible invalid socket value (for use with socket() return check).
//
#define INVALID_SOCKET                 (-1)

//
// Socket error codes.
// These values are compatible with runtime/utilities/util/Socket.c definitions.
//
#define SYS_SOCKET_ERR_UNSPECIFIED      (-1)   // Generic/unspecified error
#define SYS_SOCKET_ERR_WOULDBLOCK       (-2)   // Operation would block
#define SYS_SOCKET_ERR_ACCEPT_TIMEOUT   (-3)   // Accept operation timed out
#define SYS_SOCKET_ERR_INTERRUPT        (-5)   // Operation interrupted (EINTR)
#define SYS_SOCKET_ERR_TIMEDOUT         (-6)   // Connection timed out (ETIMEDOUT)
#define SYS_SOCKET_ERR_CONNRESET        (-7)   // Connection reset by peer (ECONNRESET)

//
// Any socket handle value.
//
#define SYS_SOCKET_IP_ADDR_ANY          0

//
// Socket option constant values
// Used with setsockopt() and ioctl() functions
//
#define SOCKOPT_ENABLE_VALUE   ((int)1)           // Enable socket option (int type)
#define SOCKOPT_DISABLE_VALUE  ((int)0)           // Disable socket option (int type)
#define NONBLOCKING_VALUE      ((unsigned long)1) // Non-blocking mode value
#define BLOCKING_VALUE         ((unsigned long)0) // Blocking mode value

//
// Shutdown modes for SYS_SOCKET_Shutdown()
// Values are identical on Windows and POSIX
//
#define SYS_SOCKET_SHUT_RD        0       // Disable further receive operations
#define SYS_SOCKET_SHUT_WR        1       // Disable further send operations
#define SYS_SOCKET_SHUT_RDWR      2       // Disable both send and receive

/*********************************************************************
*
*       Types
*
**********************************************************************
*/

/*********************************************************************
*
*       SYS_SOCKET_HANDLE
*
*  Description
*    Socket handle type for POSIX systems.
*    On Linux, this is a file descriptor (int type).
*    Negative values indicate errors or invalid handles.
*/
typedef int SYS_SOCKET_HANDLE;

/*********************************************************************
*
*       API functions
*
**********************************************************************
*/

SYS_SOCKET_HANDLE SYS_SOCKET_OpenTCP(void);
int               SYS_SOCKET_ListenAtTCPAddr(SYS_SOCKET_HANDLE hSocket, unsigned IPAddr, unsigned Port, unsigned NumConnectionsQueued);
void              SYS_SOCKET_Shutdown(SYS_SOCKET_HANDLE hSocket, int How);
void              SYS_SOCKET_Close(SYS_SOCKET_HANDLE hSocket);
int               SYS_SOCKET_IsReadable(SYS_SOCKET_HANDLE hSocket, int TimeoutMs);
int               SYS_SOCKET_IsWriteable(SYS_SOCKET_HANDLE hSocket, int TimeoutMs);
SYS_SOCKET_HANDLE SYS_SOCKET_AcceptEx(SYS_SOCKET_HANDLE hSocket, int TimeoutMs);
void              SYS_SOCKET_EnableKeepalive(SYS_SOCKET_HANDLE hSocket);
int               SYS_SOCKET_IsReady(SYS_SOCKET_HANDLE hSocket);
void              SYS_SOCKET_SetNonBlocking(SYS_SOCKET_HANDLE hSocket);
int               SYS_SOCKET_Send(SYS_SOCKET_HANDLE hSocket, const void* pData, unsigned NumBytes);
int               SYS_SOCKET_SendAll(SYS_SOCKET_HANDLE hSocket, const void* pData, unsigned NumBytes, int TimeoutMs);
int               SYS_SOCKET_Receive(SYS_SOCKET_HANDLE hSocket, void* pData, unsigned MaxNumBytes);

#if defined(__cplusplus)          // Allow usage of this module from C++ files (disable name mangling)
}                /* Make sure we have C-declarations in C++ programs */
#endif

#endif // end of include guard: TRACEHUB_SOCKET_H Avoid multiple inclusion


/*************************** End of file ****************************/
