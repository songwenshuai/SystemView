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
File    : Start_Sock.c
Purpose : TCP/IP sample application, runs a simple terminal echo server
*/

#include "RTOS.h"
#include "BSP.h"

#include <winsock2.h>
#include "iphlpapi.h"

#include "stdio.h"

/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/
#define VERSION                       10000   // Mmmrr: M = Major, mm Minor, rr Revision.  Ex.: 12301:  V1.23a
#define PORT                          23

#if DEBUG
  #define PRINTF OutputDebugString
#else
  #define PRINTF OutputDebugString
#endif

static OS_STACKPTR int StackHP[128], StackLP[128];  // Task stacks
static OS_TASK         TCBHP, TCBLP;                // Task control blocks

/*********************************************************************
*
*       _TcpIp
*
*  This function opens a connection to a socket interface
*  and communicates via the blocking send() and recv() functions.
*  This sample just functions as an echo server.
*  It can be accessed via Telnet localhost
*  The function returns when the connection is closed or when connection failed
*/
static int _TcpIp(void) {
  WSADATA       WSAData;
  WORD          VersionRequested;
  SOCKADDR_IN   sin;
  SOCKET        Sock;
  SOCKET        AcceptSock;
  char          acBuffer[256];
  int           NumBytes;
  int           r;

  PRINTF("Simple Echo server. (C) 2012 SEGGER Micrcontroller, www.segger.com\n");
  //
  // Initialize winsock. Required to use TCP/IP
  //
  VersionRequested = MAKEWORD(2, 0);
  if (WSAStartup(VersionRequested, &WSAData) != 0) {
    PRINTF("Could not init WinSock.\n");
    return 1;
  }
  //
  // Get socket and prepare it using bind() and listen()
  //
  Sock = socket(AF_INET, SOCK_STREAM, 0);
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = htonl(INADDR_ANY);
  sin.sin_port = htons(( u_short )PORT);
  if (bind( Sock, ( SOCKADDR * )&sin, sizeof( SOCKADDR_IN ))) {
    PRINTF("Could not bind socket.\n");
    return 1;
  }

  if (listen( Sock, SOMAXCONN )) {
    PRINTF("Could not listen on socket.\n");
    return 1;
  }
  //
  // Wait for connection calling accept().
  // As this is a blocking function, we use OS_SIM_EnterSysCall() and OS_SIM_LeaveSycall()
  //
  OS_SIM_EnterSysCall();
  AcceptSock = accept(Sock, 0, 0);
  OS_SIM_LeaveSysCall();
  if (!AcceptSock) {
    PRINTF("Accept failed.\n");
    return 1;
  }
  PRINTF("Connection accepted\n");
  //
  // Handle connection until it is closed or an error occurs.
  // as send() and recv() are blocking functons, we use OS_SIM_EnterSysCall() and OS_SIM_LeaveSysCall();
  //
  OS_SIM_EnterSysCall();
  send(AcceptSock, "Connected to SEGGER Sample Echo Server.\r\n", 41, 0);
  OS_SIM_LeaveSysCall();

  while (1) {
    OS_SIM_EnterSysCall();
    NumBytes = recv(AcceptSock, acBuffer, sizeof(acBuffer), 0);
    OS_SIM_LeaveSysCall();
    if (NumBytes <= 0) {
      break;
    }
    OS_SIM_EnterSysCall();
    r = send(AcceptSock, acBuffer, NumBytes, 0);
    OS_SIM_LeaveSysCall();
    if (r < NumBytes) {
      break;
    }
    BSP_ToggleLED(0);
  }
  //
  // Cleanup (not really necessary on WIN32 since the process will clean up automaticaly)
  //
  closesocket(AcceptSock);
  closesocket(Sock);
  return 0;
}

/*********************************************************************
*
*       HPTask()
*
*  The HPTask is the high priority task
*  The task calls blocking windows functions to communicate
*/
static void HPTask(void) {
  int     r;
  OS_TIME Delay;
  OS_TIME Time;

  while (1) {
    //
    // Do TCP/IP communication until connection is closed or an error occured
    //
    r = _TcpIp();
    //
    // Show result of communication
    //
    if (r == 0) {
      // Connection closed
      Delay = 40;
    } else {
      // Error occured, connectiuon could not be established
      Delay = 200;
    }
    Time = OS_TIME_GetTicks() + 2000;
    do {
      OS_TASK_Delay(Delay);
      BSP_ToggleLED(0);
    } while (Time > OS_TIME_GetTicks());
  }
}

/*********************************************************************
*
*       LPTask()
*
*  The low priority task will run, when the higher priority task is on delay,
*  or when the higher priority IPTask is blocked in a windows syscall
*/
static void LPTask(void) {
  while (1) {
    BSP_ToggleLED(1);
    OS_TASK_Delay(500);
  }
}

/*********************************************************************
*
*       main()
*/
int main(void) {
  OS_Init();    // Initialize embOS
  OS_InitHW();  // Initialize required hardware
  BSP_Init();   // Initialize LED ports
  OS_TASK_CREATE(&TCBHP, "HP Task", 100, HPTask, StackHP);
  OS_TASK_CREATE(&TCBLP, "LP Task",  50, LPTask, StackLP);
  OS_Start();   // Start embOS
  return 0;
}

/*************************** End of file ****************************/
