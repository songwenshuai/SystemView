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
----------------------------------------------------------------------
File    : Terminal_internal.h
Purpose : Internal Terminal service interfaces
---------------------------END-OF-HEADER------------------------------
*/

#ifndef TRACEHUB_TERMINAL_INTERNAL_H
#define TRACEHUB_TERMINAL_INTERNAL_H

#include <stdbool.h>
#include <stddef.h>

#include "Terminal.h"
#include "ByteQueue.h"
#include "SYS.h"
#include "Socket.h"
#include "TelnetCodec.h"

#define TERMINAL_SEND_BUF_SIZE      8192
#define TERMINAL_RECV_BUF_SIZE      8192
#define TERMINAL_RECOVERY_DELAY_MS  100
#define TERMINAL_IDLE_DELAY_MS      20

#ifndef TERMINAL_DEFAULT_NETWORK_QUEUE_SIZE
  #define TERMINAL_DEFAULT_NETWORK_QUEUE_SIZE (1024u * 1024u)
#endif

typedef struct {
    Terminal_Config_t   Config;
    bool                Initialized;
    bool                Running;
    bool                FatalError;
    bool                LockInitialized;
    SYS_SOCKET_HANDLE   hListener;
    SYS_SOCKET_HANDLE   hClient;
    SYS_Thread          ServiceThread;
    SYS_Thread          DrainThread;
    bool                ServiceThreadStarted;
    bool                DrainThreadStarted;
    bool                CoreConsumerRegistered;
    bool                ClientDisconnectRequested;
    bool                TargetInputEnabled;
    bool                RTTRecovering;
    SYS_Mutex           Lock;
    int                 SendNumBytes;
    int                 WriteNumBytes;
    char               *pSendBuf;
    char               *pWriteBuf;
    char                acSendBuf[TERMINAL_SEND_BUF_SIZE];
    char                acWriteBuf[TERMINAL_RECV_BUF_SIZE];
    char               *pNetworkQueue;
    ByteQueue_t         NetworkQueue;
    TelnetCodec_State_t TelnetState;
    unsigned            BytesSent;
    unsigned            BytesReceived;
    unsigned            ConnectionsCount;
} Terminal_State_t;

extern Terminal_State_t _terminal_state;

size_t            _Terminal_GetNetworkQueueSize(void);
bool              _Terminal_IsRunning(Terminal_State_t *pState);
void              _Terminal_SetRunning(Terminal_State_t *pState, bool running);
bool              _Terminal_HasFatalError(Terminal_State_t *pState);
void              _Terminal_ReportRTTError(Terminal_State_t *pState, const char *operation);
SYS_SOCKET_HANDLE _Terminal_GetClient(Terminal_State_t *pState);
void              _Terminal_ResetConnectionStateLocked(Terminal_State_t *pState);
void              _Terminal_ClearNetworkQueueLocked(Terminal_State_t *pState);
void              _Terminal_RequestNetworkStreamResetLocked(Terminal_State_t *pState);
void              _Terminal_ReportRecoverableRTTError(Terminal_State_t *pState, const char *operation);
void              _Terminal_ReportRTTRecovered(Terminal_State_t *pState);
unsigned          _Terminal_SetConnectedClient(Terminal_State_t *pState, SYS_SOCKET_HANDLE hClient);
void              _Terminal_AddBytesSent(Terminal_State_t *pState, unsigned NumBytes);
void              _Terminal_AddBytesReceived(Terminal_State_t *pState, unsigned NumBytes);
int               _Terminal_QueueTargetData(Terminal_State_t *pState, const char *data, unsigned num_bytes);
unsigned          _Terminal_DequeueTargetData(Terminal_State_t *pState, char *buffer, unsigned buffer_size);
int               _Terminal_RegisterCoreConsumer(Terminal_State_t *pState);
void              _Terminal_UnregisterCoreConsumer(Terminal_State_t *pState);
int               _Terminal_CheckChannels(Terminal_State_t *pState);
int               _Terminal_ReadTargetData(Terminal_State_t *pState, char *buffer, size_t buffer_size);
int               _Console_SetRawMode(void);
void              _Console_RestoreMode(void);
int               _Console_CheckInput(void);
void              _Terminal_ConsoleThread(void *pArg);
void              _Terminal_DrainThread(void *pArg);
void              _Terminal_CloseClient(Terminal_State_t *pState);
bool              _Terminal_TakeClientDisconnectRequest(Terminal_State_t *pState);
void              _Terminal_CloseClientForNetworkError(Terminal_State_t *pState, const char *operation);
void              _Terminal_ServiceThread(void *pArg);

#endif
