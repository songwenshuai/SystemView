/*********************************************************************
*                             CineLogic                              *
*                      RTT Trace and Debug Bridge                    *
**********************************************************************
----------------------------------------------------------------------
File    : TerminalApiTest.c
Purpose : Unit checks for Terminal service APIs
---------------------------END-OF-HEADER------------------------------
*/

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "Terminal_internal.h"
#include "CoreLogRecorder.h"
#include "RTTBridge.h"
#include "TestCommon.h"

#define STUB_THREAD_RESULT_COUNT 4u

static bool     _stub_core_is_core_channel;
static int      _stub_core_register_result = -1;
static int      _stub_core_read_result = -1;
static int      _stub_rtt_up_check_result;
static int      _stub_rtt_down_check_result;
static int      _stub_rtt_read_result;
static int      _stub_mutex_init_result;
static int      _stub_thread_results[STUB_THREAD_RESULT_COUNT];
static unsigned _stub_thread_call_count;
static unsigned _stub_thread_wait_count;
static int      _stub_console_raw_mode_result;
static unsigned _stub_console_restore_count;
static SYS_SOCKET_HANDLE _stub_socket_open_result;
static int      _stub_socket_listen_result;
static unsigned _stub_socket_close_count;
static unsigned _stub_socket_shutdown_count;
static unsigned _stub_core_register_count;
static unsigned _stub_core_unregister_count;
static unsigned _stub_rtt_up_check_count;
static unsigned _stub_rtt_down_check_count;
static unsigned _stub_rtt_read_count;

static void _ResetTerminalStubs(void) {
    unsigned i;

    _stub_core_is_core_channel = false;
    _stub_core_register_result = -1;
    _stub_core_read_result = -1;
    _stub_rtt_up_check_result = 0;
    _stub_rtt_down_check_result = 0;
    _stub_rtt_read_result = 0;
    _stub_mutex_init_result = 0;
    for (i = 0u; i < STUB_THREAD_RESULT_COUNT; i++) {
        _stub_thread_results[i] = -1;
    }
    _stub_thread_call_count = 0u;
    _stub_thread_wait_count = 0u;
    _stub_console_raw_mode_result = -1;
    _stub_console_restore_count = 0u;
    _stub_socket_open_result = SYS_SOCKET_INVALID_HANDLE;
    _stub_socket_listen_result = -1;
    _stub_socket_close_count = 0u;
    _stub_socket_shutdown_count = 0u;
    _stub_core_register_count = 0u;
    _stub_core_unregister_count = 0u;
    _stub_rtt_up_check_count = 0u;
    _stub_rtt_down_check_count = 0u;
    _stub_rtt_read_count = 0u;
}

void LOG_Debug(const char *file, int line, const char *function, const char *sFormat, ...) {
    (void)file;
    (void)line;
    (void)function;
    (void)sFormat;
}

void LOG_Error(const char *sFormat, ...) {
    (void)sFormat;
}

void LOG_Warn(const char *sFormat, ...) {
    (void)sFormat;
}

void LOG_Info(const char *sFormat, ...) {
    (void)sFormat;
}

int SYS_MutexInit(SYS_Mutex *mutex) {
    (void)mutex;
    return _stub_mutex_init_result;
}

int SYS_MutexLock(SYS_Mutex *mutex) {
    (void)mutex;
    return 0;
}

int SYS_MutexUnlock(SYS_Mutex *mutex) {
    (void)mutex;
    return 0;
}

int SYS_MutexDestroy(SYS_Mutex *mutex) {
    (void)mutex;
    return 0;
}

int SYS_createThread(void (*threadEntry)(void *), void *context, SYS_Thread *pRetTid) {
    int result;

    (void)threadEntry;
    (void)context;
    result = -1;
    if (_stub_thread_call_count < STUB_THREAD_RESULT_COUNT) {
        result = _stub_thread_results[_stub_thread_call_count];
    }
    _stub_thread_call_count++;
    if ((result == 0) && (pRetTid != NULL)) {
        memset(pRetTid, 0, sizeof(*pRetTid));
    }
    return result;
}

void SYS_WaitThreadTerm(SYS_Thread pRetTid) {
    (void)pRetTid;
    _stub_thread_wait_count++;
}

unsigned SYS_GetTickCount(void) {
    return 0u;
}

void SYS_Sleep(unsigned ms) {
    (void)ms;
}

bool CoreLogRecorder_IsCoreChannel(unsigned channel) {
    (void)channel;
    return _stub_core_is_core_channel;
}

int CoreLogRecorder_RegisterConsumer(unsigned channel) {
    (void)channel;
    _stub_core_register_count++;
    return _stub_core_register_result;
}

void CoreLogRecorder_UnregisterConsumer(unsigned channel) {
    (void)channel;
    _stub_core_unregister_count++;
}

int CoreLogRecorder_ReadChannel(unsigned channel, void *buffer, size_t buffer_size) {
    (void)channel;
    (void)buffer;
    (void)buffer_size;
    return _stub_core_read_result;
}

int RTTBridge_CheckUpBufferChannel(unsigned channel) {
    (void)channel;
    _stub_rtt_up_check_count++;
    return _stub_rtt_up_check_result;
}

int RTTBridge_CheckDownBufferChannel(unsigned channel) {
    (void)channel;
    _stub_rtt_down_check_count++;
    return _stub_rtt_down_check_result;
}

int RTTBridge_ReadUpBufferNoLock(unsigned channel, void *buffer, size_t buffer_size) {
    (void)channel;
    (void)buffer;
    (void)buffer_size;
    _stub_rtt_read_count++;
    return _stub_rtt_read_result;
}

SYS_SOCKET_HANDLE SYS_SOCKET_OpenTCP(void) {
    return _stub_socket_open_result;
}

void SYS_SOCKET_Close(SYS_SOCKET_HANDLE hSocket) {
    (void)hSocket;
    _stub_socket_close_count++;
}

void SYS_SOCKET_Shutdown(SYS_SOCKET_HANDLE hSocket, int How) {
    (void)hSocket;
    (void)How;
    _stub_socket_shutdown_count++;
}

int SYS_SOCKET_ListenAtTCPAddr(SYS_SOCKET_HANDLE hSocket,
                               unsigned IPAddr,
                               unsigned Port,
                               unsigned NumConnectionsQueued) {
    (void)hSocket;
    (void)IPAddr;
    (void)Port;
    (void)NumConnectionsQueued;
    return _stub_socket_listen_result;
}

void TelnetCodec_Reset(TelnetCodec_State_t *state) {
    if (state != NULL) {
        state->State = TELNET_CODEC_STATE_DATA;
        state->Command = 0u;
    }
}

int _Console_SetRawMode(void) {
    return _stub_console_raw_mode_result;
}

void _Console_RestoreMode(void) {
    _stub_console_restore_count++;
}

int _Console_CheckInput(void) {
    return 0;
}

void _Terminal_ConsoleThread(void *pArg) {
    (void)pArg;
}

void _Terminal_DrainThread(void *pArg) {
    (void)pArg;
}

void _Terminal_ServiceThread(void *pArg) {
    (void)pArg;
}

void _Terminal_CloseClient(Terminal_State_t *pState) {
    if (pState != NULL) {
        pState->hClient = SYS_SOCKET_INVALID_HANDLE;
    }
}

bool _Terminal_TakeClientDisconnectRequest(Terminal_State_t *pState) {
    (void)pState;
    return false;
}

void _Terminal_CloseClientForNetworkError(Terminal_State_t *pState, const char *operation) {
    (void)pState;
    (void)operation;
}

static int _TestTerminalLifecycle(void) {
    Terminal_Config_t config;

    _ResetTerminalStubs();
    memset(&config, 0, sizeof(config));
    _stub_mutex_init_result = -1;
    TEST_ASSERT(Terminal_Init(&config) == -1);

    _ResetTerminalStubs();
    Terminal_Stop();
    TEST_ASSERT(Terminal_Start() == -1);
    TEST_ASSERT(Terminal_Init(NULL) == -1);

    memset(&config, 0, sizeof(config));
    config.port = 19021u;
    config.channel = 1u;
    config.enabled = false;
    config.console_mode = false;
    TEST_ASSERT(Terminal_Init(&config) == 0);
    TEST_ASSERT(!Terminal_IsEnabled());
    TEST_ASSERT(!Terminal_HasFatalError());
    TEST_ASSERT(Terminal_Init(&config) == -1);
    TEST_ASSERT(Terminal_Start() == 0);
    Terminal_Status();
    Terminal_Stop();
    TEST_ASSERT(!Terminal_HasFatalError());

    config.enabled = true;
    config.console_mode = false;
    TEST_ASSERT(Terminal_Init(&config) == 0);
    TEST_ASSERT(Terminal_IsEnabled());
    TEST_ASSERT(Terminal_Start() == -1);
    Terminal_Stop();
    return 0;
}

static int _TestTerminalConsoleStartAndStop(void) {
    Terminal_Config_t config;

    _ResetTerminalStubs();
    memset(&config, 0, sizeof(config));
    config.port = 19022u;
    config.channel = 2u;
    config.enabled = true;
    config.console_mode = true;

    _stub_console_raw_mode_result = 0;
    _stub_thread_results[0] = 0;

    TEST_ASSERT(Terminal_Init(&config) == 0);
    TEST_ASSERT(Terminal_Start() == 0);
    TEST_ASSERT(_stub_thread_call_count == 1u);
    TEST_ASSERT(Terminal_IsEnabled());
    TEST_ASSERT(!Terminal_HasFatalError());
    Terminal_Status();
    Terminal_Stop();
    TEST_ASSERT(_stub_thread_wait_count == 1u);
    TEST_ASSERT(_stub_console_restore_count == 1u);
    TEST_ASSERT(!_terminal_state.Initialized);
    TEST_ASSERT(!_terminal_state.Running);
    TEST_ASSERT(!_terminal_state.LockInitialized);
    return 0;
}

static int _TestTerminalNetworkStartAndStop(void) {
    Terminal_Config_t config;

    _ResetTerminalStubs();
    memset(&config, 0, sizeof(config));
    config.port = 19023u;
    config.channel = 3u;
    config.enabled = true;
    config.console_mode = false;

    _stub_socket_open_result = (SYS_SOCKET_HANDLE)42;
    _stub_socket_listen_result = 0;
    _stub_thread_results[0] = 0;
    _stub_thread_results[1] = 0;

    TEST_ASSERT(Terminal_Init(&config) == 0);
    TEST_ASSERT(Terminal_Start() == 0);
    TEST_ASSERT(_stub_thread_call_count == 2u);
    Terminal_Status();
    Terminal_Stop();
    TEST_ASSERT(_stub_thread_wait_count == 2u);
    TEST_ASSERT(_stub_socket_close_count == 1u);
    TEST_ASSERT(!_terminal_state.Initialized);
    TEST_ASSERT(!_terminal_state.Running);
    TEST_ASSERT(!_terminal_state.LockInitialized);
    return 0;
}

static int _TestTerminalNetworkStartFailureCleanup(void) {
    Terminal_Config_t config;

    _ResetTerminalStubs();
    memset(&config, 0, sizeof(config));
    config.port = 19024u;
    config.channel = 4u;
    config.enabled = true;
    config.console_mode = false;

    _stub_socket_open_result = (SYS_SOCKET_HANDLE)43;
    _stub_socket_listen_result = -1;
    TEST_ASSERT(Terminal_Init(&config) == 0);
    TEST_ASSERT(Terminal_Start() == -1);
    TEST_ASSERT(_stub_socket_close_count == 1u);
    Terminal_Stop();

    _ResetTerminalStubs();
    memset(&config, 0, sizeof(config));
    config.port = 19025u;
    config.channel = 5u;
    config.enabled = true;
    config.console_mode = false;
    _stub_socket_open_result = (SYS_SOCKET_HANDLE)44;
    _stub_socket_listen_result = 0;
    _stub_thread_results[0] = 0;
    _stub_thread_results[1] = -1;
    TEST_ASSERT(Terminal_Init(&config) == 0);
    TEST_ASSERT(Terminal_Start() == -1);
    TEST_ASSERT(_stub_thread_call_count == 2u);
    TEST_ASSERT(_stub_thread_wait_count == 1u);
    TEST_ASSERT(_stub_socket_close_count == 1u);
    Terminal_Stop();
    return 0;
}

static int _TestTerminalStateHelpers(void) {
    Terminal_State_t state;
    char             queue_buffer[4];
    char             read_buffer[8];

    memset(&state, 0, sizeof(state));
    state.Config.channel = 3u;
    state.hClient = SYS_SOCKET_INVALID_HANDLE;

    TEST_ASSERT(!_Terminal_IsRunning(NULL));
    _Terminal_SetRunning(NULL, true);
    TEST_ASSERT(!_Terminal_HasFatalError(NULL));
    TEST_ASSERT(_Terminal_GetClient(NULL) == SYS_SOCKET_INVALID_HANDLE);

    _Terminal_SetRunning(&state, true);
    TEST_ASSERT(_Terminal_IsRunning(&state));
    _Terminal_SetRunning(&state, false);
    TEST_ASSERT(!_Terminal_IsRunning(&state));

    _Terminal_ReportRTTError(&state, NULL);
    TEST_ASSERT(_Terminal_HasFatalError(&state));
    TEST_ASSERT(!state.Running);

    memset(&state, 0, sizeof(state));
    state.Config.channel = 3u;
    ByteQueue_Init(&state.NetworkQueue, queue_buffer, sizeof(queue_buffer));
    TEST_ASSERT(ByteQueue_Write(&state.NetworkQueue, "abcd", 4u) == BYTE_QUEUE_WRITE_OK);
    _Terminal_RequestNetworkStreamResetLocked(&state);
    TEST_ASSERT(state.ClientDisconnectRequested);
    TEST_ASSERT(ByteQueue_GetUsed(&state.NetworkQueue) == 0u);

    state.SendNumBytes = 2;
    state.WriteNumBytes = 3;
    state.pSendBuf = state.acSendBuf;
    state.pWriteBuf = state.acWriteBuf;
    TEST_ASSERT(_Terminal_SetConnectedClient(&state, (SYS_SOCKET_HANDLE)7) == 1u);
    TEST_ASSERT(state.hClient == (SYS_SOCKET_HANDLE)7);
    TEST_ASSERT(!state.ClientDisconnectRequested);
    TEST_ASSERT(state.SendNumBytes == 0);
    TEST_ASSERT(state.WriteNumBytes == 0);
    TEST_ASSERT(state.pSendBuf == NULL);
    TEST_ASSERT(state.pWriteBuf == NULL);

    _Terminal_AddBytesSent(&state, 5u);
    _Terminal_AddBytesReceived(&state, 6u);
    TEST_ASSERT(state.BytesSent == 5u);
    TEST_ASSERT(state.BytesReceived == 6u);

    TEST_ASSERT(_Terminal_QueueTargetData(&state, "abc", 3u) == 0);
    TEST_ASSERT(_Terminal_DequeueTargetData(&state, read_buffer, sizeof(read_buffer)) == 3u);
    TEST_ASSERT(memcmp(read_buffer, "abc", 3u) == 0);
    TEST_ASSERT(_Terminal_QueueTargetData(&state, "abc", 3u) == 0);
    TEST_ASSERT(_Terminal_QueueTargetData(&state, "xy", 2u) == 0);
    TEST_ASSERT(state.ClientDisconnectRequested);
    TEST_ASSERT(ByteQueue_GetUsed(&state.NetworkQueue) == 2u);
    TEST_ASSERT(_Terminal_QueueTargetData(&state, "abcdef", 6u) == -1);
    TEST_ASSERT(state.FatalError);
    return 0;
}

static int _TestTerminalRecoveryAndChannelHelpers(void) {
    Terminal_State_t state;
    char             queue_buffer[4];
    char             read_buffer[8];

    _ResetTerminalStubs();
    memset(&state, 0, sizeof(state));
    state.Config.channel = 4u;
    state.hClient = (SYS_SOCKET_HANDLE)3;
    TEST_ASSERT(_Terminal_GetClient(&state) == (SYS_SOCKET_HANDLE)3);
    state.LockInitialized = true;
    state.hClient = (SYS_SOCKET_HANDLE)4;
    TEST_ASSERT(_Terminal_GetClient(&state) == (SYS_SOCKET_HANDLE)4);
    _Terminal_SetRunning(&state, true);
    TEST_ASSERT(_Terminal_IsRunning(&state));
    _Terminal_ReportRTTError(&state, "locked");
    TEST_ASSERT(_Terminal_HasFatalError(&state));
    TEST_ASSERT(!state.Running);

    memset(&state, 0, sizeof(state));
    state.Config.channel = 4u;
    state.LockInitialized = true;
    ByteQueue_Init(&state.NetworkQueue, queue_buffer, sizeof(queue_buffer));
    TEST_ASSERT(ByteQueue_Write(&state.NetworkQueue, "abcd", 4u) == BYTE_QUEUE_WRITE_OK);
    _Terminal_ReportRecoverableRTTError(&state, NULL);
    TEST_ASSERT(state.RTTRecovering);
    TEST_ASSERT(state.ClientDisconnectRequested);
    TEST_ASSERT(ByteQueue_GetUsed(&state.NetworkQueue) == 0u);
    TEST_ASSERT(!state.FatalError);
    _Terminal_ReportRecoverableRTTError(&state, "read");
    TEST_ASSERT(state.RTTRecovering);
    _Terminal_ReportRTTRecovered(&state);
    TEST_ASSERT(!state.RTTRecovering);
    _Terminal_ReportRTTRecovered(&state);
    _Terminal_ReportRecoverableRTTError(NULL, NULL);
    _Terminal_ReportRTTRecovered(NULL);

    memset(&state, 0, sizeof(state));
    state.Config.channel = 4u;
    state.Config.console_mode = true;
    ByteQueue_Init(&state.NetworkQueue, queue_buffer, sizeof(queue_buffer));
    TEST_ASSERT(ByteQueue_Write(&state.NetworkQueue, "abcd", 4u) == BYTE_QUEUE_WRITE_OK);
    _Terminal_RequestNetworkStreamResetLocked(NULL);
    _Terminal_RequestNetworkStreamResetLocked(&state);
    TEST_ASSERT(!state.ClientDisconnectRequested);
    TEST_ASSERT(ByteQueue_GetUsed(&state.NetworkQueue) == 4u);
    _Terminal_ClearNetworkQueueLocked(NULL);

    memset(&state, 0, sizeof(state));
    state.Config.channel = 4u;
    TEST_ASSERT(_Terminal_RegisterCoreConsumer(NULL) == -1);
    TEST_ASSERT(_Terminal_RegisterCoreConsumer(&state) == 0);
    TEST_ASSERT(!state.CoreConsumerRegistered);
    TEST_ASSERT(_stub_core_register_count == 0u);

    _ResetTerminalStubs();
    memset(&state, 0, sizeof(state));
    state.Config.channel = 4u;
    _stub_core_is_core_channel = true;
    _stub_core_register_result = 0;
    TEST_ASSERT(_Terminal_RegisterCoreConsumer(&state) == 0);
    TEST_ASSERT(state.CoreConsumerRegistered);
    TEST_ASSERT(_stub_core_register_count == 1u);
    _Terminal_UnregisterCoreConsumer(&state);
    TEST_ASSERT(!state.CoreConsumerRegistered);
    TEST_ASSERT(_stub_core_unregister_count == 1u);
    _Terminal_UnregisterCoreConsumer(&state);
    TEST_ASSERT(_stub_core_unregister_count == 1u);

    _ResetTerminalStubs();
    memset(&state, 0, sizeof(state));
    state.Config.channel = 4u;
    _stub_core_is_core_channel = true;
    _stub_core_register_result = -1;
    TEST_ASSERT(_Terminal_RegisterCoreConsumer(&state) == -1);
    TEST_ASSERT(!state.CoreConsumerRegistered);

    _ResetTerminalStubs();
    memset(&state, 0, sizeof(state));
    state.Config.channel = 4u;
    TEST_ASSERT(_Terminal_CheckChannels(NULL) == -1);
    _stub_rtt_up_check_result = -1;
    TEST_ASSERT(_Terminal_CheckChannels(&state) == -1);
    TEST_ASSERT(!state.TargetInputEnabled);
    TEST_ASSERT(_stub_rtt_down_check_count == 0u);

    _ResetTerminalStubs();
    _stub_rtt_down_check_result = -1;
    TEST_ASSERT(_Terminal_CheckChannels(&state) == 0);
    TEST_ASSERT(!state.TargetInputEnabled);

    _ResetTerminalStubs();
    TEST_ASSERT(_Terminal_CheckChannels(&state) == 0);
    TEST_ASSERT(state.TargetInputEnabled);

    TEST_ASSERT(_Terminal_ReadTargetData(NULL, read_buffer, sizeof(read_buffer)) == -1);
    TEST_ASSERT(_Terminal_ReadTargetData(&state, NULL, sizeof(read_buffer)) == -1);
    TEST_ASSERT(_Terminal_ReadTargetData(&state, read_buffer, 0u) == -1);

    _ResetTerminalStubs();
    state.CoreConsumerRegistered = true;
    _stub_core_read_result = 5;
    TEST_ASSERT(_Terminal_ReadTargetData(&state, read_buffer, sizeof(read_buffer)) == 5);
    state.CoreConsumerRegistered = false;
    _stub_rtt_read_result = 6;
    TEST_ASSERT(_Terminal_ReadTargetData(&state, read_buffer, sizeof(read_buffer)) == 6);
    TEST_ASSERT(_stub_rtt_read_count == 1u);

    _ResetTerminalStubs();
    return 0;
}

int main(void) {
    TEST_RUN(_TestTerminalLifecycle);
    TEST_RUN(_TestTerminalConsoleStartAndStop);
    TEST_RUN(_TestTerminalNetworkStartAndStop);
    TEST_RUN(_TestTerminalNetworkStartFailureCleanup);
    TEST_RUN(_TestTerminalStateHelpers);
    TEST_RUN(_TestTerminalRecoveryAndChannelHelpers);
    return 0;
}

/*************************** End of file ****************************/
