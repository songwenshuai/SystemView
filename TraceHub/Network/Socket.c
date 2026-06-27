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
File    : Socket.c
Purpose : TCP socket abstraction layer for Linux platform
---------------------------END-OF-HEADER------------------------------
*/

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <stdbool.h>
#include <inttypes.h>
#include <time.h>

#include <getopt.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#include "Socket.h"

/*********************************************************************
*
*       Static functions
*
**********************************************************************
*/

static uint64_t _Socket_GetTickCountMs(void) {
  struct timespec Time;

  clock_gettime(CLOCK_MONOTONIC, &Time);
  return ((uint64_t)Time.tv_sec * 1000u) + ((uint64_t)Time.tv_nsec / 1000000u);
}

/*********************************************************************
*
*       Public functions
*
**********************************************************************
*/

/*********************************************************************
*
*       SYS_SOCKET_OpenTCP
*
*  Function description
*    Creates an IPv4 TCP socket.
*
*  Return value
*    >= 0:  Socket handle
*     < 0:  SYS_SOCKET_INVALID_HANDLE on error
*
*  Notes
*    (1) TCP_NODELAY is enabled to disable Nagle's algorithm for lower latency
*/
SYS_SOCKET_HANDLE SYS_SOCKET_OpenTCP(void) {
  SYS_SOCKET_HANDLE sock;
  const int optval = SOCKOPT_ENABLE_VALUE;
  //
  // Create socket
  //
  sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock != INVALID_SOCKET) {
    //
    // Disable Nagle's algorithm for improved latency
    // Nagle's algorithm buffers small packets which increases latency
    //
    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char*)&optval, sizeof(int));
#ifdef SO_NOSIGPIPE
    setsockopt(sock, SOL_SOCKET, SO_NOSIGPIPE, (char*)&optval, sizeof(int));
#endif
  } else {
    return SYS_SOCKET_INVALID_HANDLE;
  }
  return (SYS_SOCKET_HANDLE)sock;
}

/*********************************************************************
*
*       SYS_SOCKET_ListenAtTCPAddr
*
*  Function description
*    Puts IPv4 socket into listening state.
*
*  Parameters
*    hSocket  Handle to socket that has been returned by SYS_SOCKET_OpenTCP() / SYS_SOCKET_OpenUDP()
*    IPAddr   IPv4 address expected in little endian form, meaning 127.0.0.1 is expected as 0x7F000001
*             To accept connections from any IP address, pass SYS_SOCKET_IP_ADDR_ANY
*    Port     Port to listen at
*
*  Return value
*    >= 0: O.K.
*     < 0: Error
*/
int SYS_SOCKET_ListenAtTCPAddr(SYS_SOCKET_HANDLE hSocket, unsigned IPAddr, unsigned Port, unsigned NumConnectionsQueued) {
  struct sockaddr_in addr;
  int r;
  //
  // Set SO_REUSEADDR to allow quick restart without "address already in use" error
  //
  {
    const int optval = SOCKOPT_ENABLE_VALUE;
    r = setsockopt(hSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&optval, sizeof(int));
  }
  if (r != 0) {
    return -1;
  }
  //
  // Setup IPv4 address structure
  //
  memset(&addr, 0, sizeof(addr));
  addr.sin_family      = AF_INET;
  addr.sin_port        = htons((unsigned short)Port);
  addr.sin_addr.s_addr = htonl(IPAddr);
  //
  // Bind socket to address and port
  //
  r = bind(hSocket, (struct sockaddr*)&addr, sizeof(addr));
  if (r != 0) {
    return -1;
  }
  //
  // Put socket into listening state
  //
  r = listen(hSocket, NumConnectionsQueued);
  if (r != 0) {
    return -1;
  }
  return 0;
}

/*********************************************************************
*
*       SYS_SOCKET_Shutdown
*
*  Function description
*    Shuts down part or all of a socket connection.
*
*  Parameters
*    hSocket  Handle to socket that has been returned by SYS_SOCKET_OpenTCP() / SYS_SOCKET_OpenUDP()
*    How      Shutdown mode (SYS_SOCKET_SHUT_RD, SYS_SOCKET_SHUT_WR, or SYS_SOCKET_SHUT_RDWR)
*/
void SYS_SOCKET_Shutdown(SYS_SOCKET_HANDLE hSocket, int How) {
  SYS_SOCKET_HANDLE Sock;

  Sock = (SYS_SOCKET_HANDLE)hSocket;
  switch (How) {
    case SYS_SOCKET_SHUT_RD:
      shutdown(Sock, 0);
      break;
    case SYS_SOCKET_SHUT_WR:
      shutdown(Sock, 1);
      break;
    case SYS_SOCKET_SHUT_RDWR:
      shutdown(Sock, 2);
      break;
  }
}

/*********************************************************************
*
*       SYS_SOCKET_Close
*
*  Function description
*    Closes a socket. Resources allocated by this socket are freed.
*
*  Parameters
*    hSocket  Handle to socket that has been returned by SYS_SOCKET_OpenTCP() / SYS_SOCKET_OpenUDP()
*/
void SYS_SOCKET_Close(SYS_SOCKET_HANDLE hSocket) {
  //
  // Close socket
  //
  close(hSocket);
}

/*********************************************************************
*
*       SYS_SOCKET_IsReadable
*
*  Function description
*    Checks if a socket that has been connected with SYS_SOCKET_Connect() is readable.
*
*  Parameters
*    hSocket  Handle to socket that has been returned by SYS_SOCKET_OpenTCP() / SYS_SOCKET_OpenUDP()
*
*  Return value
*    == 1  O.K., socket readable
*    == 0  O.K., socket not readable yet
*     < 0  Error
*/
int SYS_SOCKET_IsReadable(SYS_SOCKET_HANDLE hSocket, int TimeoutMs) {
  SYS_SOCKET_HANDLE Sock;
  struct timeval tv;
  fd_set rfds;
  int v;

  Sock = (SYS_SOCKET_HANDLE)hSocket;
  FD_ZERO(&rfds);       // Zero init file descriptor list
  FD_SET(Sock, &rfds);  // Add socket to file descriptor list to be monitored by select()
  tv.tv_sec = (long)(TimeoutMs / 1000);
  tv.tv_usec = (TimeoutMs % 1000) * 1000;
  v = select((hSocket + 1), &rfds, NULL, NULL, &tv);   // > 0: in case of success, == 0: Timeout, < 0: Error
  return v;
}

/*********************************************************************
*
*       SYS_SOCKET_IsWriteable
*
*  Function description
*    Checks if a socket that has been connected with SYS_SOCKET_Connect() is writeable.
*
*  Parameters
*    hSocket  Handle to socket that has been returned by SYS_SOCKET_OpenTCP() / SYS_SOCKET_OpenUDP()
*
*  Return value
*    == 1  O.K., socket writeable
*    == 0  O.K., socket not writeable yet
*/
int SYS_SOCKET_IsWriteable(SYS_SOCKET_HANDLE hSocket, int TimeoutMs) {
  SYS_SOCKET_HANDLE Sock;
  struct timeval tv;
  fd_set wfds;
  int v;
  Sock = (SYS_SOCKET_HANDLE)hSocket;
  FD_ZERO(&wfds);       // Zero init file descriptor list
  FD_SET(Sock, &wfds);  // Add socket to file descriptor list to be monitored by select()
  tv.tv_sec = (long)(TimeoutMs / 1000);
  tv.tv_usec = (TimeoutMs % 1000) * 1000;
  v = select((hSocket + 1), NULL, &wfds, NULL, &tv);   // > 0: in case of success, == 0: Timeout, < 0: Error
  return v;
}

/*********************************************************************
*
*       SYS_SOCKET_AcceptEx
*
*  Function description
*    Waits for a connection (with timeout) on the given socket.
*
*  Parameters
*    hSocket   Handle to socket that has been returned by SYS_SOCKET_OpenTCP() / SYS_SOCKET_OpenUDP()
*    TimeoutMs Timeout in ms for waiting
*
*  Return value
*    >= 0  Handle to socket of new connection that has been established
*     < 0  Error   (SYS_SOCKET_INVALID_HANDLE)
*      -2  Timeout (SYS_SOCKET_ERR_ACCEPT_TIMEOUT)
*/
SYS_SOCKET_HANDLE SYS_SOCKET_AcceptEx(SYS_SOCKET_HANDLE hSocket, int TimeoutMs) {
  SYS_SOCKET_HANDLE SockChild;
  struct sockaddr_in  sockAddr;
  const int nodelay = SOCKOPT_ENABLE_VALUE;
  int r;
  int len;
  //
  // Validate socket handle
  //
  if (hSocket < 0) {
    return SYS_SOCKET_INVALID_HANDLE;
  }
  //
  // accept() itself does not allow using timeouts
  // Therefore we check readability first and then call accept() which should not block then
  //
  len = sizeof(struct sockaddr_in);
  r = SYS_SOCKET_IsReadable(hSocket, TimeoutMs);
  if (r < 0) {
    return SYS_SOCKET_INVALID_HANDLE; // error
  } else if (r == 0) {
    return SYS_SOCKET_ERR_ACCEPT_TIMEOUT; // timeout
  } else {
    SockChild = accept(hSocket, (struct sockaddr*)&sockAddr, (socklen_t *)&len);
    if (SockChild != INVALID_SOCKET) {
      //
      // Disable Nagle's algorithm to speed things up
      // Nagle's algorithm prevents small packets from being transmitted and collects some time until data is actually sent out
      //
      setsockopt(SockChild, IPPROTO_TCP, TCP_NODELAY, (char*)&nodelay, sizeof(int));
#ifdef SO_NOSIGPIPE
      setsockopt(SockChild, SOL_SOCKET, SO_NOSIGPIPE, (char*)&nodelay, sizeof(int));
#endif
    } else {
      return SYS_SOCKET_INVALID_HANDLE;
    }
  }
  return (SYS_SOCKET_HANDLE)SockChild;
}

/*********************************************************************
*
*       SYS_SOCKET_EnableKeepalive
*
*/
void SYS_SOCKET_EnableKeepalive(SYS_SOCKET_HANDLE hSocket) {
  int on = 1;
  setsockopt(hSocket, SOL_SOCKET, SO_KEEPALIVE, &on, sizeof(int));
}

/*********************************************************************
*
*       SYS_SOCKET_IsReady
*
*  Function description
*    Checks if a socket has no pending connection error.
*
*  Parameters
*    hSocket  Handle to socket that has been returned by SYS_SOCKET_OpenTCP() / SYS_SOCKET_OpenUDP()
*
*  Return value
*    == 1  O.K., socket ready
*    == 0  O.K., socket not ready yet
*     < 0  Error
*/
int SYS_SOCKET_IsReady(SYS_SOCKET_HANDLE hSocket) {
  int error;
  socklen_t len;

  if (hSocket < 0) {
    return SYS_SOCKET_ERR_UNSPECIFIED;
  }

  error = 0;
  len = sizeof(error);
  if (getsockopt(hSocket, SOL_SOCKET, SO_ERROR, &error, &len) != 0) {
    return SYS_SOCKET_ERR_UNSPECIFIED;
  }
  if ((error == EINPROGRESS) || (error == EALREADY)) {
    return 0;
  }
  if (error != 0) {
    return -error;
  }
  return 1;
}

/*********************************************************************
*
*       SYS_SOCKET_SetNonBlocking
*
*  Function description
*    Sets socket to non-blocking mode.
*
*  Parameters
*    hSocket  Handle to socket
*
*  Notes
*    (1) Uses FIONBIO (0x5421) ioctl command
*    (2) In non-blocking mode, operations return immediately with EWOULDBLOCK if data not available
*/
void SYS_SOCKET_SetNonBlocking(SYS_SOCKET_HANDLE hSocket) {
  unsigned long mode = NONBLOCKING_VALUE;
  ioctl(hSocket, FIONBIO, &mode);
}

/*********************************************************************
*
*       SYS_SOCKET_Send
*
*  Function description
*    Sends data on the specified socket
*
*  Parameters
*    hSocket    Handle to socket that has been returned by SYS_SOCKET_OpenTCP() / SYS_SOCKET_OpenUDP()
*    pData      Pointer to data buffer to send
*    NumBytes   Number of bytes to send
*
*  Return value
*    >= 0:  Number of bytes sent
*     < 0:  Error code (SYS_SOCKET_ERR_*)
*           SYS_SOCKET_ERR_WOULDBLOCK - Socket would block (non-blocking mode)
*           SYS_SOCKET_ERR_UNSPECIFIED - Other errors (including invalid parameters)
*
*  Notes
*    (1) Uses non-blocking send flags when available.
*    (2) Suppresses SIGPIPE when the platform provides a socket-level mechanism.
*    (3) Returns immediately with EWOULDBLOCK if send buffer is full.
*    (4) May send fewer bytes than requested (check return value).
*/
int SYS_SOCKET_Send(SYS_SOCKET_HANDLE hSocket, const void* pData, unsigned NumBytes) {
  int r;
  int Err;
  int Flags;
  SYS_SOCKET_HANDLE Sock;
  //
  // Validate input parameters
  //
  if (pData == NULL) {
    return SYS_SOCKET_ERR_UNSPECIFIED;
  }
  //
  // Perform send operation
  //
  Sock = (SYS_SOCKET_HANDLE)hSocket;
  //
  // Use per-call non-blocking and SIGPIPE suppression flags when available.
  //
  Flags = 0;
#ifdef MSG_DONTWAIT
  Flags |= MSG_DONTWAIT;
#endif
#ifdef MSG_NOSIGNAL
  Flags |= MSG_NOSIGNAL;
#endif
  r = send(Sock, pData, NumBytes, Flags);
  if (r < 0) {
    Err = errno;
    switch (Err) {
#if EAGAIN != EWOULDBLOCK
    case EAGAIN:
#endif
    case EWOULDBLOCK:
      r = SYS_SOCKET_ERR_WOULDBLOCK;
      break;
    default:
      r = SYS_SOCKET_ERR_UNSPECIFIED;
      break;
    }
  }
  return r;
}

/*********************************************************************
*
*       SYS_SOCKET_SendAll
*
*  Function description
*    Sends the complete buffer before returning.
*
*  Parameters
*    hSocket    Handle to socket
*    pData      Pointer to data buffer to send
*    NumBytes   Number of bytes to send
*    TimeoutMs  Total timeout in milliseconds, 0 for a non-waiting attempt
*
*  Return value
*    >= 0:  Number of bytes sent, always NumBytes on success
*     < 0:  Error code (SYS_SOCKET_ERR_*)
*/
int SYS_SOCKET_SendAll(SYS_SOCKET_HANDLE hSocket, const void* pData, unsigned NumBytes, int TimeoutMs) {
  const unsigned char *p;
  unsigned             Sent;
  uint64_t             StartMs;
  uint64_t             ElapsedMs;
  int                  WaitMs;
  int                  r;

  if ((hSocket < 0) || (pData == NULL) || (TimeoutMs < 0) || (NumBytes > (unsigned)INT_MAX)) {
    return SYS_SOCKET_ERR_UNSPECIFIED;
  }
  if (NumBytes == 0u) {
    return 0;
  }

  p = (const unsigned char *)pData;
  Sent = 0u;
  StartMs = _Socket_GetTickCountMs();

  while (Sent < NumBytes) {
    if (TimeoutMs > 0) {
      ElapsedMs = _Socket_GetTickCountMs() - StartMs;
      if (ElapsedMs >= (uint64_t)TimeoutMs) {
        return SYS_SOCKET_ERR_TIMEDOUT;
      }
      WaitMs = TimeoutMs - (int)ElapsedMs;
      if (WaitMs > 100) {
        WaitMs = 100;
      }

      r = SYS_SOCKET_IsWriteable(hSocket, WaitMs);
      if (r < 0) {
        return SYS_SOCKET_ERR_UNSPECIFIED;
      }
      if (r == 0) {
        continue;
      }
    }

    r = SYS_SOCKET_Send(hSocket, p + Sent, NumBytes - Sent);
    if (r > 0) {
      Sent += (unsigned)r;
      continue;
    }
    if (r == 0) {
      return SYS_SOCKET_ERR_TIMEDOUT;
    }
    if (r == SYS_SOCKET_ERR_WOULDBLOCK) {
      if (TimeoutMs == 0) {
        return r;
      }
      continue;
    }
    return r;
  }

  return (int)Sent;
}

/*********************************************************************
*
*       SYS_SOCKET_Receive
*
*  Function description
*    Receives data on the given socket.
*
*  Parameters
*    hSocket      Handle to socket returned by SYS_SOCKET_OpenTCP() / SYS_SOCKET_OpenUDP()
*    pData        Pointer to buffer to receive data
*    MaxNumBytes  Maximum number of bytes to receive
*
*  Return value
*    >= 0:  Number of bytes received
*     < 0:  Error code (SYS_SOCKET_ERR_*)
*           SYS_SOCKET_ERR_WOULDBLOCK - No data available (non-blocking mode)
*           SYS_SOCKET_ERR_TIMEDOUT   - Operation timed out
*           SYS_SOCKET_ERR_CONNRESET  - Connection reset by peer
*           SYS_SOCKET_ERR_INTERRUPT  - Interrupted by signal
*           SYS_SOCKET_ERR_UNSPECIFIED - Other errors
*
*  Notes
*    (1) Returns as soon as data is received (may be less than MaxNumBytes)
*    (2) Error codes are mapped from system errno to portable error codes
*/
int SYS_SOCKET_Receive(SYS_SOCKET_HANDLE hSocket, void* pData, unsigned MaxNumBytes) {
  int r;
  int Err;
  SYS_SOCKET_HANDLE Sock;

  Sock = (SYS_SOCKET_HANDLE)hSocket;
  r = recv(Sock, (char*)pData, MaxNumBytes, 0);
  if (r < 0) {
    Err = errno;
    switch (Err) {
    case EINTR:
      r = SYS_SOCKET_ERR_INTERRUPT;
      break;
#if EAGAIN != EWOULDBLOCK
    case EAGAIN:
#endif
    case EWOULDBLOCK:
      r = SYS_SOCKET_ERR_WOULDBLOCK;
      break;
    case ENETRESET:
    case ECONNRESET:
      r = SYS_SOCKET_ERR_CONNRESET;
      break;
    case ETIMEDOUT:
      r = SYS_SOCKET_ERR_TIMEDOUT;
      break;
    default:
      r = SYS_SOCKET_ERR_UNSPECIFIED;
    }
  }
  return r;
}

/*************************** End of file ****************************/
