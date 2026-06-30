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
File    : IPHlpApi.h
Purpose : IP interface
*/

#ifndef IPHLPAPI_H                // Avoid multiple inclusion
#define IPHLPAPI_H

#include "GLOBAL.h"

/*********************************************************************
*
*       Defines
*
**********************************************************************
*/

#define MAX_ADAPTER_DESC_LEN      128
#define MAX_ADAPTER_NAME_LEN      256
#define MAX_ADAPTER_ADDR_LEN      8

#define MIB_IF_TYPE_OTHER         1
#define MIB_IF_TYPE_ETHERNET      6
#define MIB_IF_TYPE_TOKENRING     9
#define MIB_IF_TYPE_FDDI          15
#define MIB_IF_TYPE_PPP           23
#define MIB_IF_TYPE_LOOPBACK      24
#define MIB_IF_TYPE_SLIP          28

/*********************************************************************
*
*       Types
*
**********************************************************************
*/

typedef struct IP_ADDR_STRUCT {
  struct IP_ADDR_STRUCT* pNext;
  char      acIPAddr[16];
  char      acIPMask[16];
  U32       Context;
} IP_ADDR;

typedef struct IP_ADAPTER_INFO_STRUCT {
  struct IP_ADAPTER_INFO_STRUCT* pNext;
  U32       ComboIndex;
  char      acName[MAX_ADAPTER_NAME_LEN + 4];
  char      acDesc[MAX_ADAPTER_DESC_LEN + 4];
  U32       AddressLength;
  U8        Address[MAX_ADAPTER_ADDR_LEN];
  U32       Index;
  U32       Type;
  U32       DHCPEnabled;
  IP_ADDR*  pCurrentIPAddr;
  IP_ADDR   IPAddrList;
  IP_ADDR   GatewayList;
  IP_ADDR   DHCPServer;
  U32       HaveWins;
  IP_ADDR   PrimaryWinsServer;
  IP_ADDR   SecondaryWinsServer;
  U32       LeaseObtained;
  U32       LeaseExpires;
} IP_ADAPTER_INFO;

/*********************************************************************
*
*       API
*
**********************************************************************
*/

U32 __stdcall GetAdaptersInfo(IP_ADAPTER_INFO* pInfo, U32* pBufferSize);

#endif

/*************************** end of file ****************************/
