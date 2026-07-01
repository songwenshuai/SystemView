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
File    : UDP_Sim_Process.c
Purpose : UPD communication for embOSView and embOS Simulation.
*/

#if !defined(EMBOS_SIM_HOST_WINDOWS) || !defined(EMBOS_SIM_HOST_POSIX)
  #error "EMBOS_SIM_HOST_WINDOWS and EMBOS_SIM_HOST_POSIX must be defined by the simulation build."
#endif

#if defined(__linux__)
  #define _GNU_SOURCE
#endif

#if (EMBOS_SIM_HOST_WINDOWS != 0)

#include <Windows.h>
#include "RTOS.h"
#include "UDPCOM.h"
#include <stdio.h>
#if (OS_SUPPORT_TRACE_API != 0)
#include "SEGGER_SYSVIEW_Win32.h"
#endif

/*********************************************************************
*
*       Configuration
*
**********************************************************************
*/
#ifndef   EMBOSVIEW_UDP_PORT_RX         // This is the default UDP port for embOSView communication.
  #define EMBOSVIEW_UDP_PORT_RX  50021  // If you change it please modify it also in embOSView.
#endif

//
// Only if we run multiple embOS instances on the same IP address (e.g. multiple embOS simulation running on one PC)
// it is necessary to define distinct port numbers.
// Otherwise we use the client port number from the received UDP packet.
// Per default embOSView uses the next free distinct port number for the local port.
//
//#define EMBOSVIEW_UDP_PORT_TX  (EMBOSVIEW_UDP_PORT_RX + 1)  // We use the next port number to reply to the client

/*********************************************************************
*
*       Defines
*
**********************************************************************
*/
#define PROTOCOL_SIZE  5  // Size of embOSView protocol bytes (SD0, SD1, Size, Checksum, ED)

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/
static SOCKADDR_IN _ServerAddr;
static SOCKADDR_IN _ClientAddr;
static SOCKET      _sock;
static HANDLE      _hThread;
static char        _aRxData[1024];

/*********************************************************************
*
*       Local functions
*
**********************************************************************
*/

/*********************************************************************
*
*       _OnRx()
*
*  Function description
*    UDP RX Thread
*/
static void _OnRx(void) {
  SOCKADDR_IN SockAddr;
  int i;
  int NumRxBytes;
  int Len;

  OS_SIM_SetThreadName(-1, "embOSView Communication");
#if (OS_SUPPORT_TRACE_API != 0)
  SEGGER_SYSVIEW_X_SetISRName("embOSView Communication");
#endif
  while (1) {
    memset(_aRxData, 0, sizeof(_aRxData));
    //
    // Receive packed, get IP address and UDP port from incoming packet
    //
    SockAddr = _ServerAddr;
    Len  = sizeof(SockAddr);
    NumRxBytes = recvfrom(_sock, _aRxData, sizeof(_aRxData), 0, (SOCKADDR*)&SockAddr, &Len);
    _ClientAddr.sin_addr.s_addr = SockAddr.sin_addr.s_addr;
#ifndef EMBOSVIEW_UDP_PORT_TX
    _ClientAddr.sin_port        = SockAddr.sin_port;
#endif
    //
    // Handle received embOSView protocol
    //
    for (i = 0u; i < NumRxBytes; i++) {
      OS_INT_Enter();
      OS_COM_OnRx(_aRxData[i]);
      OS_INT_Leave();
    }
  }
}

/*********************************************************************
*
*       Global functions
*
**********************************************************************
*/

/*********************************************************************
*
*       UDP_Process_Send1()
*
*  Function description
*    Sends the UDP protocol
*/
void UDP_Process_Send1(char c) {
  static char OutPacket[0x500];
  OS_INT      SD1;
  OS_INT      NumBytes;
  OS_INT      i;
  OS_INT      Offset;
  OS_INT      NumTxBytes;

  Offset = 0;
  //
  // Send UDP packet only if we already got a target address
  //
  if (_ClientAddr.sin_addr.s_addr != 0u) {
    SD1      = OS_COM_GetNextChar();  // Get SD1
    NumBytes = OS_COM_GetNextChar();  // Get NumBytes
    if (NumBytes >= 0xF0) {
      NumBytes = ((NumBytes & 0x0F) << 8u) | (OS_COM_GetNextChar() & 0xFF);
      Offset   = 1;
    }
    //
    // Alloc packet, we need a packet for the amount of data bytes plus protocol bytes
    //
    memset(OutPacket, 0, sizeof(OutPacket));
    //
    // Copy protocol bytes and data in out UDP packet
    //
    OutPacket[0] = c;
    OutPacket[1] = SD1;
    if (NumBytes >= 0xF0) {
      OutPacket[2] = (NumBytes >> 8) | 0xF0;
      OutPacket[3] = NumBytes & 0xFF;
    } else {
      OutPacket[2] = NumBytes;
    }

    for (i = 0; i < NumBytes + 2 + Offset; i++) {
      OutPacket[i + 3 + Offset] = OS_COM_GetNextChar();
    }

    NumTxBytes = sendto(_sock, OutPacket, NumBytes + PROTOCOL_SIZE + Offset, 0, (SOCKADDR*)&_ClientAddr, sizeof(_ClientAddr));
    if (NumTxBytes == -1) {
      OutputDebugString("sendto error\n");
    }
    do {
      i = OS_COM_GetNextChar(); // Call the state machine until there are no more characters to send
    } while (i >= 0);
  } else {
    //
    // No target address so far, discard response
    //
    OS_COM_ClearTxActive();
  }
}

/*********************************************************************
*
*       UDP_Process_Init()
*
*  Function description
*    Initializes the UDP communication for embOSView
*/
void UDP_Process_Init(void) {
  WORD        wVersionRequested;
  WSADATA     wsaData;
  SOCKADDR_IN _ServerAddr;
  int         Result;
  char        sText[100];

  memset(&_ServerAddr, 0, sizeof(_ServerAddr));
  memset(&_ClientAddr, 0, sizeof(_ClientAddr));
  _ClientAddr.sin_family = AF_INET;
#ifdef EMBOSVIEW_UDP_PORT_TX
  _ClientAddr.sin_port   = htons(EMBOSVIEW_UDP_PORT_TX);
#endif
  //
  // Initialize winsock. Required to use UDP
  //
  wVersionRequested = MAKEWORD(2, 0);
  if (WSAStartup(wVersionRequested, &wsaData) != 0) {
    OutputDebugString("Could not init WinSock.\n");
  }
  //
  // Create datagram socket
  //
  _sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (_sock == INVALID_SOCKET) {
    OutputDebugString ("Could not create socket.\n");
  }
  //
  // Bind Rx port
  //
  _ServerAddr.sin_family      = AF_INET;
  _ServerAddr.sin_port        = htons(EMBOSVIEW_UDP_PORT_RX);
  _ServerAddr.sin_addr.s_addr = INADDR_ANY;
  Result = bind(_sock, (struct sockaddr *)&_ServerAddr, sizeof(_ServerAddr));
  if (Result != SOCKET_ERROR) {
    //
    // Create thread for incoming UDP packets
    //
    _hThread = (HANDLE)OS_SIM_CreateISRThreadEx(_OnRx, "embOSView Communication");

    sprintf(sText, "Please configure embOSView to your local IP addres and port number %u.\n", EMBOSVIEW_UDP_PORT_RX);
    OutputDebugString(sText);
  } else {
    OutputDebugString("Could not bind socket.\n");
  }
}

#elif (EMBOS_SIM_HOST_POSIX != 0)

#include "RTOS.h"
#include "UDPCOM.h"
#if (OS_SUPPORT_TRACE_API != 0)
#include "SEGGER_SYSVIEW_Posix.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>

/*********************************************************************
*
*       Configuration
*
**********************************************************************
*/
#ifndef   EMBOSVIEW_UDP_PORT_RX         // This is the default UDP port for embOSView communication.
  #define EMBOSVIEW_UDP_PORT_RX  50021  // If you change it please modify it also in embOSView.
#endif

//
// Only if we run multiple embOS instances on the same IP address (e.g. multiple embOS simulation running on one PC)
// it is necessary to define distinct port numbers.
// Otherwise we use the client port number from the received UDP packet.
// Per default embOSView uses the next free distinct port number for the local port.
//
//#define EMBOSVIEW_UDP_PORT_TX  (EMBOSVIEW_UDP_PORT_RX + 1)  // We use the next port number to reply to the client

/*********************************************************************
*
*       Defines
*
**********************************************************************
*/
#define PROTOCOL_SIZE         (5)  // Size of embOSView protocol bytes (SD0, SD1, Size, Checksum, ED)
#define HANDLE_ERROR(e, msg)  do { errno = e; perror(msg); exit(EXIT_FAILURE); } while (0)

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/
static pthread_mutex_t    _Mutex = PTHREAD_MUTEX_INITIALIZER;
static struct sockaddr_in _ServerAddr;
static struct sockaddr_in _ClientAddr;
static int                _Socket;
static char               _aRxData[1024];
static int                _NumRxBytes;

/*********************************************************************
*
*       Global data
*
**********************************************************************
*/
void* _embOSViewISRHandle;

/*********************************************************************
*
*       Local functions
*
**********************************************************************
*/

/*********************************************************************
*
*       _OnRxController()
*
*  Function description
*    This function controls the occurrence of RX interrupts.
*/
static int _OnRxController(void) {
#if (OS_SUPPORT_TRACE_API != 0)
  static int         InitOnce = 1;
#endif
  struct sockaddr_in SockAddr;
  socklen_t          Len;

#if (OS_SUPPORT_TRACE_API != 0)
  if (InitOnce != 0) {
    InitOnce = 0;
    SEGGER_SYSVIEW_X_SetISRName("embOSView Communication");
  }
#endif
  //
  // Receive incoming packets until at least 1 byte was received.
  //
  do {
    Len = sizeof(SockAddr);
    _NumRxBytes = recvfrom(_Socket, _aRxData, sizeof(_aRxData), 0, (struct sockaddr*)&SockAddr, &Len);
  } while (_NumRxBytes < 1);
  //
  // Update IP address and port of the embOSView client.
  //
  pthread_mutex_lock(&_Mutex);
  _ClientAddr.sin_addr.s_addr = SockAddr.sin_addr.s_addr;
#ifndef EMBOSVIEW_UDP_PORT_TX
  _ClientAddr.sin_port = SockAddr.sin_port;
#endif
  pthread_mutex_unlock(&_Mutex);
  return 1;
}

/*********************************************************************
*
*       _OnRxHandler()
*/
static void _OnRxHandler(void) {
  int i;

  OS_INT_Enter();
  for (i = 0; i < _NumRxBytes; ++i) {
    OS_COM_OnRx(_aRxData[i]);
  }
  OS_INT_Leave();
}

/*********************************************************************
*
*       Global functions
*
**********************************************************************
*/

/*********************************************************************
*
*       UDP_Process_Send1()
*
*  Function description
*    Sends the UDP protocol.
*/
void UDP_Process_Send1(char c) {
  static char        OutPacket[0x500];
  OS_INT             SD1;
  OS_INT             NumBytes;
  OS_INT             i;
  OS_INT             Offset;
  OS_INT             NumTxBytes;
  struct sockaddr_in ClientAddr;

  Offset = 0;
  //
  // Send UDP packet only if we already got a target address
  //
  pthread_mutex_lock(&_Mutex);
  memcpy(&ClientAddr, &_ClientAddr, sizeof (ClientAddr));
  pthread_mutex_unlock(&_Mutex);
  if (ClientAddr.sin_addr.s_addr != 0u) {
    SD1      = OS_COM_GetNextChar();  // Get SD1
    NumBytes = OS_COM_GetNextChar();  // Get NumBytes
    if (NumBytes >= 0xF0) {
      NumBytes = ((NumBytes & 0x0F) << 8u) | (OS_COM_GetNextChar() & 0xFF);
      Offset   = 1;
    }
    //
    // Alloc packet, we need a packet for the amount of data bytes plus protocol bytes
    //
    memset(OutPacket, 0, sizeof(OutPacket));
    //
    // Copy protocol bytes and data in out UDP packet
    //
    OutPacket[0] = c;
    OutPacket[1] = SD1;
    if (NumBytes >= 0xF0) {
      OutPacket[2] = (NumBytes >> 8) | 0xF0;
      OutPacket[3] = NumBytes & 0xFF;
    } else {
      OutPacket[2] = NumBytes;
    }
    for (i = 0; i < NumBytes + 2 + Offset; i++) {
      OutPacket[i + 3 + Offset] = OS_COM_GetNextChar();
    }
    //
    // Send packet.
    //
    NumTxBytes = sendto(_Socket, OutPacket, NumBytes + PROTOCOL_SIZE + Offset, 0, (struct sockaddr*)&ClientAddr, sizeof(ClientAddr));
    if (NumTxBytes == -1) {
      fprintf(stderr, "sendto error\n");
    }
    //
    // Call the state machine until there are no more characters to send.
    //
    do {
      i = OS_COM_GetNextChar();
    } while (i >= 0);
  } else {
    //
    // No target address so far, discard response
    //
    OS_COM_ClearTxActive();
  }
}

/*********************************************************************
*
*       UDP_Process_Init()
*
*  Function description
*    Initializes the UDP communication for embOSView.
*/
void UDP_Process_Init(void) {
  int Result;

  if (_embOSViewISRHandle == NULL) {
    memset(&_ServerAddr, 0, sizeof(_ServerAddr));
    memset(&_ClientAddr, 0, sizeof(_ClientAddr));
    _ClientAddr.sin_family = AF_INET;
#ifdef EMBOSVIEW_UDP_PORT_TX
    _ClientAddr.sin_port   = htons(EMBOSVIEW_UDP_PORT_TX);
#endif
    //
    // Create datagram socket
    //
    _Socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (_Socket < 0) {
      HANDLE_ERROR(errno, "socket");
    }
    //
    // Bind Rx port
    //
    _ServerAddr.sin_family      = AF_INET;
    _ServerAddr.sin_port        = htons(EMBOSVIEW_UDP_PORT_RX);
    _ServerAddr.sin_addr.s_addr = INADDR_ANY;
    Result = bind(_Socket, (struct sockaddr *)&_ServerAddr, sizeof(_ServerAddr));
    if (Result != 0) {
      HANDLE_ERROR(errno, "bind");
    }
    //
    // Create thread for incoming UDP packets
    //
    _embOSViewISRHandle = OS_SIM_CreateISRThreadEx(_OnRxHandler, _OnRxController, "embOSView Com");
  }
}

#else
  #error "Unsupported simulation platform."
#endif

/*************************** End of file ****************************/
