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
File    : SystemView_internal.h
Purpose : Internal SystemView service interfaces
---------------------------END-OF-HEADER------------------------------
*/

#ifndef TRACEHUB_SYSTEMVIEW_INTERNAL_H
#define TRACEHUB_SYSTEMVIEW_INTERNAL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "SystemView.h"
#include "ByteQueue.h"
#include "SYS.h"
#include "Socket.h"

#define SYSVIEW_SEND_BUF_SIZE       8192
#define SYSVIEW_RECV_BUF_SIZE       8192
#define SYSVIEW_APP_PACKET_MAX_SIZE 255u
#define SYSVIEW_IDLE_DELAY_MS       20
#define SYSVIEW_HANDSHAKE_TIMEOUT_MS 5000
#define SYSVIEW_RECORD_FILE_EXTENSION "SVDat"
#define SYSVIEW_RECORD_FLUSH_INTERVAL_MS 100
#define SYSVIEW_RECORD_FLUSH_THRESHOLD (64u * 1024u)
#define SYSVIEW_RECOVERY_DELAY_MS   100

#ifndef SYSTEMVIEW_DEFAULT_NETWORK_QUEUE_SIZE
  #define SYSTEMVIEW_DEFAULT_NETWORK_QUEUE_SIZE (1024u * 1024u)
#endif

#if (SYSVIEW_APP_PACKET_MAX_SIZE > SYSVIEW_RECV_BUF_SIZE)
  #error "SYSVIEW_APP_PACKET_MAX_SIZE must fit in the receive buffer"
#endif

typedef struct {
    SystemView_Config_t Config;
    bool                Initialized;
    bool                Running;
    bool                FatalError;
    bool                RecordFileError;
    bool                LockInitialized;
    SYS_SOCKET_HANDLE   hListener;
    SYS_SOCKET_HANDLE   hClient;
    SYS_Thread          ServiceThread;
    SYS_Thread          RecordThread;
    bool                ServiceThreadStarted;
    bool                RecordThreadStarted;
    SYS_Mutex           Lock;
    bool                HandshakeDone;
    bool                ClientDisconnectRequested;
    bool                RTTRecovering;
    FILE               *record_file;
    int                 SendNumBytes;
    int                 WriteNumBytes;
    unsigned            AppPacketExpected;
    unsigned            AppPacketReceived;
    char               *pSendBuf;
    char               *pWriteBuf;
    char                acSendBuf[SYSVIEW_SEND_BUF_SIZE];
    char                acWriteBuf[SYSVIEW_RECV_BUF_SIZE];
    char               *pNetworkQueue;
    ByteQueue_t         NetworkQueue;
    unsigned            BytesSent;
    unsigned            BytesReceived;
    unsigned            ConnectionsCount;
} SystemView_State_t;

extern SystemView_State_t _sysview_state;

size_t            _SystemView_GetNetworkQueueSize(void);
bool              _SystemView_IsRunning(SystemView_State_t *pState);
void              _SystemView_SetRunning(SystemView_State_t *pState, bool running);
bool              _SystemView_HasFatalError(SystemView_State_t *pState);
void              _SystemView_ReportFatalServiceError(SystemView_State_t *pState, const char *operation);
void              _SystemView_ReportRecoverableRTTError(SystemView_State_t *pState, const char *operation);
void              _SystemView_ReportRecordFileError(SystemView_State_t *pState, const char *operation, int saved_errno);
int               _SystemView_WriteRecordFileHeader(SystemView_State_t *pState, FILE *record_file);
int               _SystemView_FlushRecordFile(SystemView_State_t *pState, FILE *record_file);
void              _SystemView_ClearNetworkQueueLocked(SystemView_State_t *pState);
void              _SystemView_RequestNetworkStreamResetLocked(SystemView_State_t *pState);
void              _SystemView_ReportRTTRecovered(SystemView_State_t *pState);
int               _SystemView_CheckChannels(SystemView_State_t *pState);
SYS_SOCKET_HANDLE _SystemView_GetClient(SystemView_State_t *pState);
void              _SystemView_ResetConnectionStateLocked(SystemView_State_t *pState);
unsigned          _SystemView_SetConnectedClient(SystemView_State_t *pState, SYS_SOCKET_HANDLE hClient);
void              _SystemView_AddBytesSent(SystemView_State_t *pState, unsigned NumBytes);
void              _SystemView_AddBytesReceived(SystemView_State_t *pState, unsigned NumBytes);
bool              _SystemView_PerformHandshake(SystemView_State_t *pState, SYS_SOCKET_HANDLE hSock);
int               _SystemView_QueueTargetData(SystemView_State_t *pState, const char *data, unsigned num_bytes);
unsigned          _SystemView_DequeueTargetData(SystemView_State_t *pState, char *buffer, unsigned buffer_size);
void              _SystemView_CloseClient(SystemView_State_t *pState);
bool              _SystemView_TakeClientDisconnectRequest(SystemView_State_t *pState);
void              _SystemView_CloseClientForNetworkError(SystemView_State_t *pState, const char *operation);
void              _SystemView_RecordingThread(void *pArg);
void              _SystemView_ServiceThread(void *pArg);

#endif
