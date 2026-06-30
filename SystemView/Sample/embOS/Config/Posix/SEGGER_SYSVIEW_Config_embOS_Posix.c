/*********************************************************************
*                     SEGGER Microcontroller GmbH                    *
*                        The Embedded Experts                        *
**********************************************************************
*                                                                    *
*       (c) 1995 - 2026 SEGGER Microcontroller GmbH                  *
*                                                                    *
*       Internet: segger.com  Support: support_embos@segger.com      *
*                                                                    *
**********************************************************************
*                                                                    *
*       embOS-Classic * Real time operating system                   *
*                                                                    *
*       Please note:                                                 *
*                                                                    *
*       Knowledge of this file may under no circumstances            *
*       be used to write a similar product or a real-time            *
*       operating system for in-house use.                           *
*                                                                    *
*       Thank you for your fairness !                                *
*                                                                    *
**********************************************************************
*                                                                    *
*       OS version: V5.22.0.0                                        *
*                                                                    *
**********************************************************************

-------------------------- END-OF-HEADER -----------------------------
File    : SEGGER_SYSVIEW_Config_embOS_Posix.c
Purpose : Sample setup configuration of SystemView with embOS.
*/

#define _GNU_SOURCE

#include "RTOS.h"
#include "SEGGER_SYSVIEW.h"
#include "SEGGER_SYSVIEW_Conf.h"
#include "SEGGER_SYSVIEW_embOS.h"
#include "SEGGER_SYSVIEW_Posix.h"
#include "SEGGER_RTT.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>

/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/
// The application name to be displayed in SystemViewer
#ifndef   SYSVIEW_APP_NAME
  #define SYSVIEW_APP_NAME        "embOS start project"
#endif

// The target device name
#ifndef   SYSVIEW_DEVICE_NAME
  #define SYSVIEW_DEVICE_NAME     "Simulation"
#endif

// Frequency of the timestamp. Must match SEGGER_SYSVIEW_Conf.h
#ifndef   SYSVIEW_TIMESTAMP_FREQ
  #define SYSVIEW_TIMESTAMP_FREQ  (1000u)
#endif

// System Frequency. SystemcoreClock is used in most CMSIS compatible projects.
#ifndef   SYSVIEW_CPU_FREQ
  #define SYSVIEW_CPU_FREQ        (1000000u)
#endif

// Define as 1 to immediately start recording after initialization to catch system initialization.
#ifndef   SYSVIEW_START_ON_INIT
  #define SYSVIEW_START_ON_INIT   0
#endif

#ifndef   MAX_ISRNAMES_LENGTH
  #define MAX_ISRNAMES_LENGTH     400
#endif

/*********************************************************************
*
*       Defines, fixed
*
**********************************************************************
*/



#define SYSVIEW_COMM_APP_HELLO_SIZE     32
#define SYSVIEW_COMM_TARGET_HELLO_SIZE  32

#define HANDLE_ERROR(e, msg)            do { errno = e; perror(msg); exit(EXIT_FAILURE); } while (0)

#define _INVALID_SOCKET                 (-1)

#define _SYS_THREAD_CREATE_SUSPENDED    (1)

#define _SYS_SOCKET_INVALID_HANDLE      (-1)
#define _SYS_SOCKET_IP_ADDR_ANY         0
#define _SYS_SOCKET_IP_ADDR_LOCALHOST   0x7F000001                  // 127.0.0.1 (localhost)

#define _SYS_SOCKET_PORT_ANY            0

#define _SYS_SOCKET_ERR_UNSPECIFIED     -1
#define _SYS_SOCKET_ERR_WOULDBLOCK      -2
#define _SYS_SOCKET_ERR_TIMEDOUT        -3
#define _SYS_SOCKET_ERR_CONNRESET       -4
#define _SYS_SOCKET_ERR_INTERRUPT       -5

#define _SYS_SOCKET_SHUT_RD             0
#define _SYS_SOCKET_SHUT_WR             1
#define _SYS_SOCKET_SHUT_RDWR           2

/*********************************************************************
*
*       Types, local
*
**********************************************************************
*/
typedef void* _SYS_HANDLE;
typedef int   _SYS_SOCKET_HANDLE;
typedef void* _SYS_THREAD_PROC_EX(void*);

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/
// "Hello" message expected by SystemView App: SEGGER SystemView VM.mm.rr
static const U8     _abHelloMsg[SYSVIEW_COMM_TARGET_HELLO_SIZE] = { 'S', 'E', 'G', 'G', 'E', 'R', ' ', 'S', 'y', 's', 't', 'e', 'm', 'V', 'i', 'e', 'w', ' ', 'V', '0' + SEGGER_SYSVIEW_MAJOR, '.', '0' + (SEGGER_SYSVIEW_MINOR / 10), '0' + (SEGGER_SYSVIEW_MINOR % 10), '.', '0' + (SEGGER_SYSVIEW_REV / 10), '0' + (SEGGER_SYSVIEW_REV % 10), '\0', 0, 0, 0, 0, 0 };
static volatile int _CloseRequested;            // Indicator for threads to terminate themselves
static volatile int _SysViewCommThreadRunning;  // Indicator for status of the "SysView communication" thread

static int          _int1 = 1;
static char         _sISRNames[MAX_ISRNAMES_LENGTH];

/*********************************************************************
*
*       Local functions, SYS
*
**********************************************************************
*/

/*********************************************************************
*
*       _OpenSocketNoInherit
*
*  Function description
*    Opens a socket and sets the SOCK_CLOEXEC flag,
*    so that the socket handle will not be inherited by child processes.
*
*  Parameters
*    See doc. for socket()
*
*  Return value
*    See doc. for socket()
*/
static int _OpenSocketNoInherit(int Domain, int Type, int Protocol) {
  int hSock;
  //
  // (200117) LG:
  // Testing the functionality of SOCK_CLOEXEC when opening sockets has shown that it does not seem to work correctly on
  // Ubuntu 18.04 LTS (64bit)
  //
  // --> One more reason to have child processes on Linux close any open file descriptor's that they might have inherited on start
  //
  Type |= SOCK_CLOEXEC;                    // This flag is supported since Linux 2.6.27 (Oct 2008) (http://man7.org/linux/man-pages/man2/socket.2.html)
  hSock = socket(Domain, Type, Protocol);
  if (hSock == _INVALID_SOCKET) {
    goto Done;
  }
Done:
  return hSock;
}

/*********************************************************************
*
*       SYS_Sleep
*/
static void _SYS_Sleep(int ms) {
  usleep(ms * 1000);
}

/*********************************************************************
*
*       SYS_GetLastError
*/
static U32 _SYS_GetLastError(void) {
  return errno;
}

/*********************************************************************
*
*       _SetThreadName
*/
static void _SetThreadName(pthread_t Thread, const char* sThreadName) {
  char sName[16];

  strncpy(sName, sThreadName, 15);
  sName[15] = '\0';
  pthread_setname_np(Thread, sName);
}

/*********************************************************************
*
*       SYS_CreateThreadEx
*/
static _SYS_HANDLE _SYS_CreateThreadEx(_SYS_THREAD_PROC_EX* pfThreadProc, void* pPara, U64* pThreadId, const char* sName, U32 Flags) {
  pthread_t          Thread;
  pthread_attr_t     Attr;
  int                Policy;
  int                Result;
  struct sched_param SchedulerParameters;
  struct rlimit      ResourceLimit;

  SEGGER_USE_PARA(Flags);
  Thread = pthread_self();
  Result = pthread_getattr_np(Thread, &Attr);
  if (Result != 0) {
    HANDLE_ERROR(Result, "pthread_getattr_np");
  }
  Result = pthread_attr_getschedpolicy(&Attr, &Policy);
  if (Result != 0) {
    HANDLE_ERROR(Result, "pthread_attr_getschedpolicy");
  }
  Result = pthread_attr_init(&Attr);
  if (Result != 0) {
    HANDLE_ERROR(Result, "pthread_attr_init");
  }
  Result = pthread_attr_setdetachstate(&Attr, PTHREAD_CREATE_DETACHED);
  if (Result != 0) {
    HANDLE_ERROR(Result, "pthread_attr_setdetachstate");
  }
  if (Policy == SCHED_FIFO) {
    Result = pthread_attr_setinheritsched(&Attr, PTHREAD_EXPLICIT_SCHED);
    if (Result != 0) {
      HANDLE_ERROR(Result, "pthread_attr_setinheritsched");
    }
    Result = getrlimit(RLIMIT_RTPRIO, &ResourceLimit);
    if (Result != 0) {
      HANDLE_ERROR(errno, "getrlimit");
    }
    Result = pthread_attr_setschedpolicy(&Attr, SCHED_FIFO);
    if (Result != 0) {
      HANDLE_ERROR(Result, "pthread_attr_setschedpolicy");
    }
    SchedulerParameters.sched_priority = ResourceLimit.rlim_max;
    Result = pthread_attr_setschedparam(&Attr, &SchedulerParameters);
    if (Result != 0) {
      HANDLE_ERROR(Result, "pthread_attr_setschedparam");
    }
  }
  Result = pthread_create(&Thread, &Attr, pfThreadProc, pPara);
  if (Result != 0) {
    HANDLE_ERROR(Result, "pthread_create");
  }
  if (sName) {
    _SetThreadName(Thread, sName);
  }
  if (pThreadId) {
    *pThreadId = (U64)(intptr_t)Thread;
  }
  return (_SYS_HANDLE)(Thread);
}

/*********************************************************************
*
*       _SYS_SOCKET_OpenTCP
*
*  Function description
*    Creates an IPv4 TCP socket.
*
*  Return value
*    Handle to socket
*/
static _SYS_SOCKET_HANDLE _SYS_SOCKET_OpenTCP(void) {
  int sock;
  //
  // Create socket
  //
  sock = _OpenSocketNoInherit(AF_INET, SOCK_STREAM, 0);
  if (sock != _INVALID_SOCKET) {
    //
    // Disable Nagle's algorithm to speed things up
    // Nagle's algorithm prevents small packets from being transmitted and collects some time until data is actually sent out
    //
    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char*)&_int1, sizeof(int));
  } else {
    sock = _SYS_SOCKET_INVALID_HANDLE;
  }
  return (_SYS_SOCKET_HANDLE)sock;
}

/*********************************************************************
*
*       _SYS_SOCKET_Close
*
*  Function description
*    Closes a socket. Resources allocated by this socket are freed.
*
*  Parameters
*    hSocket  Handle to socket that has been returned by _SYS_SOCKET_OpenTCP() / _SYS_SOCKET_OpenUDP()
*/
static void _SYS_SOCKET_Close(_SYS_SOCKET_HANDLE hSocket) {
  int sock;
  //
  // Close socket
  //
  sock = (int)hSocket;
  close(sock);
}

/*********************************************************************
*
*       _SYS_SOCKET_ListenAtTCPAddr
*
*  Function description
*    Puts IPv4 socket into listening state.
*
*  Parameters
*    hSocket  Handle to socket that has been returned by _SYS_SOCKET_OpenTCP() / _SYS_SOCKET_OpenUDP()
*    IPAddr   IPv4 address expected in little endian form, meaning 127.0.0.1 is expected as 0x7F000001
*             To accept connections from any IP address, pass _SYS_SOCKET_IP_ADDR_ANY
*    Port     Port to listen at
*
*  Return value
*    >= 0: O.K.
*     < 0: Error
*/
static int _SYS_SOCKET_ListenAtTCPAddr(_SYS_SOCKET_HANDLE hSocket, U32 IPAddr, U32 Port, unsigned NumConnectionsQueued) {
  int r;
  int sock;
  struct sockaddr_in addr;
  //
  // Option SO_REUSEADDR:
  //
  // <IPAddr>: IP addresses of network adapters
  //
  // ==================================================
  // Original idea from BSD sockets
  // ==================================================
  // bind() without SO_REUSEADDR set (default):
  // bind(SockA, 0.0.0.0:21)
  // bind(SockB, 192.168.0.1:21)
  // SockB will fail because <IPAddr> of SockA is a wildcard that means "Any local address",
  // so it is not possible to bind to any other local address with the same port.
  // bind(SockA, 127.0.0.1:21)
  // bind(SockB, 192.168.0.1:21)
  // Both calls will succeed, as different <IPAddr>:<Port> combinations are used.
  //
  // bind() with SO_REUSEADDR set:
  // bind(SockA, 0.0.0.0:21)
  // bind(SockB, 192.168.0.1:21)
  // SockA and SockB will succeed.
  // The original idea includes that *each* of the sockets must have SO_REUSEADDR set before bind().
  // If only the second one calls it, bind() will fail as the first socket did not allow sharing at all.
  //
  // This was the original idea of SO_REUSEADDR.
  // NOTE: Not sure who really ever needed this, but that's the way it is...
  //
  // ==================================================
  // Second effect of SO_REUSEADDR (TIME_WAIT)
  // ==================================================
  // There is another case where this option has an effect on:
  // Calls to send() do not guarantee that data is sent when the function returns. It may be sent delayed.
  // Therefore, it is possible that when calling close() to close a socket, send data is still pending.
  // What the OS does is: preparing everything for closing the connection and return from close().
  // Now the socket changed it's state from ACTIVE to TIME_WAIT but still exists inside the OS (not accessible for the user anymore)
  // If now a new socket is opened and a bind() is performed on exactly the <IPAddr>:<Port> combination of the TIME_WAIT socket, the behavior depends on if the original socket had SO_REUSEADDR set.
  //
  // SO_REUSEADDR not set:
  // bind() will fail as TIME_WAIT is handled as if it is ACTIVE
  //
  // SO_REUSEADDR set:
  // bind() will succeed as TIME_WAIT is handled as if socket was not existing anymore.
  // NOTE: Under rare circumstances, it now can happen that if there is any receive data arriving late at the system, the new socket that did the bind(), will receive it.
  //
  // ==================================================
  // OS specifics
  // ==================================================
  //
  // Windows:
  // When specifying SO_REUSEADDR before bind(), Windows will report SUCCESS on bind(),
  // even if there is another ACTIVE socket that is bound to the same <IPAddr>:<Port> combination.
  // It does not matter if the process that did the first bind() did specify SO_REUSEADDR for its socket or not
  // This allows processes to steal data from other ones...pretty awful bug in Windows... (See MSDN: Using SO_REUSEADDR and SO_EXCLUSIVEADDRUSE)
  // Microsoft introduced SO_EXCLUSIVEADDRUSE for this.
  // This makes sure hijacking the socket data is not possible.
  // But it still allows the special behavior in case a bind() to a closed socket in TIME_WAIT state is possible. (See "Second effect of SO_REUSEADDR (TIME_WAIT)" above)
  //
  // Linux:
  // Listening socket:
  // SO_REUSEADDR does not have any effect for the "original idea" (see above). Linux is more restrictive than BSD sockets here.
  // But it has the desired effect on closed sockets in TIME_WAIT state (see above).
  //
  // Client socket:
  // Behaves like the original BSD idea and has the TIME_WAIT effect
  //
  // Kernel >= 3.9: To have "original idea" effect for listening sockets, since kernel 3.9 SO_REUSEPORT has been introduced.
  //
  // Normal TCP connection close:
  // Client1 (C1), Client2 (C2)
  // C1 -> C2  FIN
  // C1 <- C2  ACK
  // C1 <- C2  FIN
  // C1 -> C2  ACK
  // Socket of C1 (as the initiator of the close request) now is in TIME_WAIT state and can stay there several seconds/minutes
  // so the <IPAddr>:<Port> combination is blocked for some time, after the socket has been closed
  // As this is not acceptable for us as the DLL and other J-Link utilities must be able to be started / terminated multiple times in a row,
  // we make use of SO_REUSEADDR for all of our listener sockets which need to bind() to a specific port
  //
  sock = (int)hSocket;
  r = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)&_int1, sizeof(int));
  if (r == 0) {
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons((U16)Port);
    addr.sin_addr.s_addr = htonl(IPAddr);
    r = bind(sock, (struct sockaddr*)&addr, sizeof(addr));
    if (r == 0) {
      r = listen(sock, NumConnectionsQueued);
    } else {
      r = -1;
    }
  } else {
    r = -1;
  }
  return r;
}

/*********************************************************************
*
*       _SYS_SOCKET_IsReady
*
*  Function description
*    Checks if a socket that has been connected with _SYS_SOCKET_Connect() is ready.
*    Mainly used on non-blocking sockets to check if they are ready to operate on.
*    The procedure (non-blocking connect, then trying FIONREAD) is recommended by MS (MSDN).
*
*  Parameters
*    hSocket  Handle to socket that has been returned by _SYS_SOCKET_OpenTCP() / _SYS_SOCKET_OpenUDP()
*
*  Return value
*    == 1  O.K., socket ready
*    == 0  O.K., socket not ready yet
*/
static unsigned _SYS_SOCKET_IsReady(_SYS_SOCKET_HANDLE hSocket) {
  int sock;
  int IsReady;
  unsigned long v;

  sock = (int)hSocket;
  ioctl(sock, FIONREAD, &v);     // Check if socket is ready to read from
  IsReady = v ? 1 : 0;
  return IsReady;
}

/*********************************************************************
*
*       _SYS_SOCKET_IsReadable
*
*  Function description
*    Checks if a socket that has been connected with _SYS_SOCKET_Connect() is readable.
*
*  Parameters
*    hSocket  Handle to socket that has been returned by _SYS_SOCKET_OpenTCP() / _SYS_SOCKET_OpenUDP()
*
*  Return value
*    == 1  O.K., socket readable
*    == 0  O.K., socket not readable yet
*     < 0  Error
*/
static int _SYS_SOCKET_IsReadable(_SYS_SOCKET_HANDLE hSocket, int TimeoutMs) {
  int Sock;
  struct timeval tv;
  fd_set rfds;
  int v;

  Sock = (int)hSocket;
  FD_ZERO(&rfds);       // Zero init file descriptor list
  FD_SET(Sock, &rfds);  // Add socket to file descriptor list to be monitored by select()
  tv.tv_sec = (long)(TimeoutMs / 1000);
  tv.tv_usec = (TimeoutMs % 1000) * 1000;
  v = select(Sock + 1, &rfds, NULL, NULL, &tv);   // > 0: in case of success, == 0: Timeout, < 0: Error
  return v;
}

/*********************************************************************
*
*       _SYS_SOCKET_AcceptEx
*
*  Function description
*    Waits for a connection (with timeout) on the given socket.
*
*  Parameters
*    hSocket   Handle to socket that has been returned by _SYS_SOCKET_OpenTCP() / _SYS_SOCKET_OpenUDP()
*    TimeoutMs Timeout in ms for waiting
*
*  Return value
*    >= 0  Handle to socket of new connection that has been established
*     < 0  Error   (_SYS_SOCKET_INVALID_HANDLE)
*      -2  Timeout
*/
static _SYS_SOCKET_HANDLE _SYS_SOCKET_AcceptEx(_SYS_SOCKET_HANDLE hSocket, int TimeoutMs) {
  int r;
  //
  // accept() itself does not allow using timeouts
  // Therefore we use a combination of select() to wait for the socket to become ready (with timeout)
  // and then call accept() which should not block then.
  //
  if ((int)hSocket < 0) {   // Some GLibc versions throw exceptions when calling socket functions with a negative handle.
    return _SYS_SOCKET_INVALID_HANDLE;
  }
  r = _SYS_SOCKET_IsReadable(hSocket, TimeoutMs);
  if (r < 0) {
    r = _SYS_SOCKET_INVALID_HANDLE; // error
  } else if (r == 0) {
    r = -2;                        // timeout
  } else {
    r = accept((int)hSocket, NULL, NULL);
    if (r >= 0) {
      //
      // Disable Nagle's algorithm to speed things up
      // Nagle's algorithm prevents small packets from being transmitted and collects some time until data is actually sent out
      //
      setsockopt(r, IPPROTO_TCP, TCP_NODELAY, (char*)&_int1, sizeof(int));
    } else {
      r = _SYS_SOCKET_INVALID_HANDLE;
    }
  }
  return (_SYS_SOCKET_HANDLE)r;
}

/*********************************************************************
*
*       _SYS_SOCKET_Receive
*
*  Function description
*    Receives data on the given socket.
*
*  Parameters
*    hSocket          Handle to socket that has been returned by _SYS_SOCKET_OpenTCP() / _SYS_SOCKET_OpenUDP()
*
*  Return value
*    >= 0:  O.K., number of bytes received
*     < 0:  Error, see _SYS_SOCKET_ERR_*
*
*  Notes
*    (1) Returns as soon as something has been received (may be less than MaxNumBytes) or error happened
*/
static int _SYS_SOCKET_Receive(_SYS_SOCKET_HANDLE hSocket, void* pData, U32 MaxNumBytes) {
  int sock;
  int Err;
  int r;

  sock = (int)hSocket;
  r = recv(sock, pData, MaxNumBytes, 0);
  if (r < 0) {
    Err = errno;
    switch (Err) {
#if EAGAIN != EWOULDBLOCK
    case EAGAIN:
#endif
    case EWOULDBLOCK:
      r = _SYS_SOCKET_ERR_WOULDBLOCK;
      break;
    case ENETRESET:
      r = _SYS_SOCKET_ERR_CONNRESET;
      break;
    case ETIMEDOUT:
      r = _SYS_SOCKET_ERR_TIMEDOUT;
      break;
    case EINTR:
      r = _SYS_SOCKET_ERR_INTERRUPT;
      break;
    default:
      r = _SYS_SOCKET_ERR_UNSPECIFIED;
    }
  }
  return r;
}

/*********************************************************************
*
*       _SYS_SOCKET_IsWriteable
*
*  Function description
*    Checks if a socket that has been connected with _SYS_SOCKET_Connect() is writeable.
*
*  Parameters
*    hSocket  Handle to socket that has been returned by _SYS_SOCKET_OpenTCP() / _SYS_SOCKET_OpenUDP()
*
*  Return value
*    == 1  O.K., socket writeable
*    == 0  O.K., socket not writeable yet
*/
static int _SYS_SOCKET_IsWriteable(_SYS_SOCKET_HANDLE hSocket, int TimeoutMs) {
  int Sock;
  struct timeval tv;
  fd_set wfds;
  int v;

  Sock = (int)hSocket;
  FD_ZERO(&wfds);       // Zero init file descriptor list
  FD_SET(Sock, &wfds);  // Add socket to file descriptor list to be monitored by select()
  tv.tv_sec = (long)(TimeoutMs / 1000);
  tv.tv_usec = (TimeoutMs % 1000) * 1000;
  v = select(Sock + 1, NULL, &wfds, NULL, &tv);   // > 0: in case of success, == 0: Timeout, < 0: Error
  return v;
}

/*********************************************************************
*
*       _SYS_SOCKET_Send
*
*  Function description
*    Sends data on the specified socket
*
*  Parameters
*    hSocket          Handle to socket that has been returned by _SYS_SOCKET_OpenTCP() / _SYS_SOCKET_OpenUDP()
*
*  Return value
*    >= 0:  O.K., number of bytes sent
*     < 0:  Error
*     See _SYS_SOCKET_ERR_
*/
static int _SYS_SOCKET_Send(_SYS_SOCKET_HANDLE hSocket, const void* pData, U32 NumBytes) {
  int sock;
  int Err;
  int r;

  sock = (int)hSocket;
  //
  // Sending without MSG_NOSIGNAL can cause a SIGPIPE to be sent if the
  // connection was closed by peer and our process would be killed.
  //
  r = send(sock, pData, NumBytes, MSG_NOSIGNAL);
  if (r < 0) {
    Err = errno;
    if ((Err == EAGAIN) || (Err == EWOULDBLOCK)) {
      r = _SYS_SOCKET_ERR_WOULDBLOCK;
    } else {
      r = _SYS_SOCKET_ERR_UNSPECIFIED;
    }
  }
  return r;
}

/*********************************************************************
*
*       Local functions, SystemView
*
**********************************************************************
*/
/*********************************************************************
*
*       _cbSendSystemDesc()
*
*  Function description
*    Sends SystemView description strings.
*/
static void _cbSendSystemDesc(void) {
  SEGGER_SYSVIEW_SendSysDesc("N=" SYSVIEW_APP_NAME ",O=embOS,D=" SYSVIEW_DEVICE_NAME );
  if (strlen(_sISRNames) > 0) {
    SEGGER_SYSVIEW_SendSysDesc(_sISRNames);
  }
}

/*********************************************************************
*
*       Local functions, SystemView Communication Channel
*
**********************************************************************
*/
/*********************************************************************
*
*       _SysViewCommThread
*
*  Function description
*    Function that handles TCP/IP connection and communication with SysView.
*
*  Parameters
*    pPara    Expected to be the ID of the RTT <Up> channel used by SysView
*/
static void* _SysViewCommThread(void* pPara) {
  _SYS_SOCKET_HANDLE hSockListen;
  _SYS_SOCKET_HANDLE hSockSV;
  int Result;
  int ChannelID;
  int v;
  int r;
  int NumBytes;
  char acBuf[2048];

  _SysViewCommThreadRunning = 1;
  Result = 0;
  ChannelID = (int)pPara;
  hSockSV   = _SYS_SOCKET_INVALID_HANDLE;
  v = 0;
  //
  // Try and connect to SystemView instance
  //
  hSockListen = _SYS_SOCKET_OpenTCP();
  if (hSockListen == _SYS_SOCKET_INVALID_HANDLE) {  // Failed to open socket? => Done
    Result = -1;
    goto Done;
  }
  r = _SYS_SOCKET_ListenAtTCPAddr(hSockListen, _SYS_SOCKET_IP_ADDR_ANY, 19111, 1);
  if (r < 0) {                                     // Failed to set socket to listening? => Done
    Result = -1;
    goto Done;
  }
  //
  // After a succesful connection, poll RTT buffer for data
  //
  do {
    if (_CloseRequested) {        // Close requested? => Stop systemview session
      if (r == 1) {               // Systemview session running?
        r = _SYS_SOCKET_IsWriteable(hSockSV, 10);
        if (r == 1 && v == 1) {   // TCP/IP connection still established? => Stop systemview session
          r = SEGGER_RTT_ReadUpBufferNoLock(SEGGER_SYSVIEW_RTT_CB_ADDRESS, ChannelID, acBuf, sizeof(acBuf));
          if (r > 0) {            // Read RTT data? => Send it via TCP/IP
            NumBytes = _SYS_SOCKET_Send(hSockSV, acBuf, r);  // We do not care if the send succeeded or not as we are closing anyway.
            _SYS_Sleep(10);        // Give system view some time to receive <Stop> info before closing socket.
          }
        }
      }
      goto Done;
    }
    if (hSockSV > _SYS_SOCKET_INVALID_HANDLE) {
      r = _SYS_SOCKET_IsWriteable(hSockSV, 10);
      if (r == 0) {   // Timeout
        continue;
      } else if (r < 0) { // Error
        _SYS_SOCKET_Close(hSockSV);
        hSockSV = _SYS_SOCKET_INVALID_HANDLE;
        continue;
      }
    } else {
      hSockSV = _SYS_SOCKET_AcceptEx(hSockListen, 100);
      if (hSockSV < 0) {
        continue;
      }
      r = _SYS_SOCKET_IsReady(hSockSV);
      if (r != 1) {               // Failed to connect? => Try again later
        continue;
      }
      //
      // Successful connection? => Start systemview session
      // First, receive <Hello> message from SysView and send back own <Hello> message
      //
      r = _SYS_SOCKET_Receive(hSockSV, acBuf, SYSVIEW_COMM_APP_HELLO_SIZE);
      if (r != SYSVIEW_COMM_APP_HELLO_SIZE) {
        printf(" --- Failed to receive \"Hello\" message from SysView...\n");
      }
      r = _SYS_SOCKET_Send(hSockSV, _abHelloMsg, SYSVIEW_COMM_TARGET_HELLO_SIZE);
      if (r != SYSVIEW_COMM_TARGET_HELLO_SIZE) {
        printf(" --- Failed to send \"Hello\" message to SysView...\n");
      }
    }
    //
    // Connection established? => Handle communication
    // Check for data sent by SysView
    //
    r = _SYS_SOCKET_IsReadable(hSockSV, 0);
    if (r == 1) {                                 // Data to read from SysView available?
      r = _SYS_SOCKET_Receive(hSockSV, acBuf, 1);  // Receive <NumBytes> to read
      if (r != 1) {                               // Failed to receive data? => Connection lost
        _SYS_SOCKET_Close(hSockSV);
        hSockSV = _SYS_SOCKET_INVALID_HANDLE;
        continue;
      }
      v = acBuf[0];
      r = _SYS_SOCKET_Receive(hSockSV, acBuf, v);  // Receive all data
      if (r != v) {                               // Failed to receive data? => Connection lost
        _SYS_SOCKET_Close(hSockSV);
        hSockSV = _SYS_SOCKET_INVALID_HANDLE;
        continue;
      }
      NumBytes = SEGGER_RTT_WriteDownBufferNoLock(SEGGER_SYSVIEW_RTT_CB_ADDRESS, ChannelID, &acBuf[0], r);  // Write data into corresponding RTT buffer for application to read and handle accordingly
    }
    //
    // Check for data to send to SysView
    //
    NumBytes = SEGGER_RTT_ReadUpBufferNoLock(SEGGER_SYSVIEW_RTT_CB_ADDRESS, ChannelID, &acBuf[0], sizeof(acBuf));
    if (NumBytes > 0) {                               // Data to send available?
      r = _SYS_SOCKET_Send(hSockSV, acBuf, NumBytes);  // Send data to SysView
      if (NumBytes != r) {                            // Failed to send data? => Connection lost
        v = _SYS_GetLastError();
        _SYS_SOCKET_Close(hSockSV);
        hSockSV = _SYS_SOCKET_INVALID_HANDLE;
      }
    }
    _SYS_Sleep(1);                                     // Sleep for some time before polling again
  } while (1);
Done:
  //
  // Clean up
  //
  if (hSockSV >= 0) {
    _SYS_SOCKET_Close(hSockSV);
  }
  if (hSockListen >= 0) {
    _SYS_SOCKET_Close(hSockListen);
  }
  _SysViewCommThreadRunning = 0;
  return (void*)Result;
}

/*********************************************************************
*
*       _SetupComm()
*
*  Function description
*    Setup communication channel.
*/
static void _SetupComm(void) {
  int r;
  U64 ThreadID;
  //
  // Initialize SysView communication
  //
  r = SEGGER_SYSVIEW_GetChannelID();                                              // Retrieve the ID of the RTT <Up> / <Down> channel used by SysView
  _SYS_CreateThreadEx(_SysViewCommThread, (void*)r, &ThreadID, "SystemView", 0);  // Start thread handling TCP/IP connection and communication with SysView instance
}

/*********************************************************************
*
*       Global functions
*
**********************************************************************
*/

/*********************************************************************
*
*       SEGGER_SYSVIEW_Conf()
*
*  Function description
*    Configure SystemView and embOS for use of SystemView.
*/
void SEGGER_SYSVIEW_Conf(void) {
  SEGGER_SYSVIEW_Init(1000000000, 1000000, &SYSVIEW_X_OS_TraceAPI, _cbSendSystemDesc);
  OS_SetTraceAPI(&embOS_TraceAPI_SYSVIEW);    // Configure embOS to use SYSVIEW.
#if SYSVIEW_START_ON_INIT
  SEGGER_SYSVIEW_Start();                     // Start recording to catch system initialization.
#endif

  _SetupComm();
}

/*********************************************************************
*
*       SEGGER_SYSVIEW_X_GetTimestamp()
*
*  Function description
*    Get the timestamp for SystemView.
*    On Windows use the performance counter.
*/
U32 SEGGER_SYSVIEW_X_GetTimestamp(void) {
  struct timespec Timespec;
  OS_U64          Timestamp;

  clock_gettime(CLOCK_MONOTONIC, &Timespec);
  Timestamp  = Timespec.tv_sec * 1000000000;
  Timestamp += Timespec.tv_nsec;
  return Timestamp;
}

/*********************************************************************
*
*       SEGGER_SYSVIEW_X_GetInterruptId()
*
*  Function description
*    Get the "dummy" interrupt ID.
*/
U32 SEGGER_SYSVIEW_X_GetInterruptId(void) {
  return pthread_self();
}

/*********************************************************************
*
*       SEGGER_SYSVIEW_X_SetISRName()
*
*  Function description
*    Informs SystemView about an ISR name.
*
*  Parameters
*    sName: ISR Name
*
*  Additional information
*    Must be called from an ISR after SEGGER_SYSVIEW_Conf() only.
*    It uses the thread ID as an unique ID for SystemView.
*/
void SEGGER_SYSVIEW_X_SetISRName(const char* sName) {
  char s[100];

  //
  // Check whether the string fits in the string buffer.
  //
  if (strlen(sName) < (sizeof(s) - 10)) {
    OS_INT_IncDI();
    //
    // If this is the first entry we don't need the comma.
    //
    if (strlen(_sISRNames) == 0) {
      sprintf(s, "I#%u=%s", (unsigned int)pthread_self(), sName);
    } else {
      sprintf(s, ",I#%u=%s", (unsigned int)pthread_self(), sName);
    }
    //
    // Add new ISR name to the ISR name string and inform SystemView (if enough space is left in the string buffer).
    //
    if ((strlen(_sISRNames) + strlen(s) + 1) < MAX_ISRNAMES_LENGTH) {
      strcat(_sISRNames, s);
      //
      // Send new description if SystemView is started.
      //
      if (SEGGER_SYSVIEW_IsStarted() > 0) {
        SEGGER_SYSVIEW_SendSysDesc(_sISRNames);
      }
    }
    OS_INT_DecRI();
  }
}

/*************************** End of file ****************************/
