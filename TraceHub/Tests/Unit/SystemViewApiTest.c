/*********************************************************************
*                             CineLogic                              *
*                      RTT Trace and Debug Bridge                    *
**********************************************************************
----------------------------------------------------------------------
File    : SystemViewApiTest.c
Purpose : Unit checks for SystemView service APIs
---------------------------END-OF-HEADER------------------------------
*/

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "SystemView_internal.h"
#include "RTTBridge.h"
#include "TestCommon.h"

#define STUB_THREAD_RESULT_COUNT 4u

static int      _stub_rtt_up_check_result;
static int      _stub_rtt_down_check_result;
static int      _stub_rtt_region_result;
static int      _stub_rtt_read_result;
static int      _stub_mutex_init_result;
static int      _stub_thread_results[STUB_THREAD_RESULT_COUNT];
static unsigned _stub_thread_call_count;
static unsigned _stub_thread_wait_count;
static bool     _stub_log_create_file_fail;
static SYS_SOCKET_HANDLE _stub_socket_open_result;
static int      _stub_socket_listen_result;
static unsigned _stub_socket_close_count;
static unsigned _stub_socket_shutdown_count;
static bool     _stub_stop_recording_on_read;
static unsigned _stub_stop_recording_after_reads;
static bool     _stub_stop_recording_on_sleep;
static unsigned _stub_sleep_count;
static unsigned _stub_rtt_up_check_count;
static unsigned _stub_rtt_down_check_count;
static unsigned _stub_rtt_read_count;

static void _ResetSystemViewStubs(void) {
    unsigned i;

    _stub_rtt_up_check_result = 0;
    _stub_rtt_down_check_result = 0;
    _stub_rtt_region_result = 0;
    _stub_rtt_read_result = 0;
    _stub_mutex_init_result = 0;
    for (i = 0u; i < STUB_THREAD_RESULT_COUNT; i++) {
        _stub_thread_results[i] = -1;
    }
    _stub_thread_call_count = 0u;
    _stub_thread_wait_count = 0u;
    _stub_log_create_file_fail = false;
    _stub_socket_open_result = SYS_SOCKET_INVALID_HANDLE;
    _stub_socket_listen_result = -1;
    _stub_socket_close_count = 0u;
    _stub_socket_shutdown_count = 0u;
    _stub_stop_recording_on_read = false;
    _stub_stop_recording_after_reads = 0u;
    _stub_stop_recording_on_sleep = false;
    _stub_sleep_count = 0u;
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

FILE *LOG_CreateTimestampedFileEx(const char *prefix, const char *extension, const char *mode) {
    (void)prefix;
    (void)extension;
    (void)mode;
    if (_stub_log_create_file_fail) {
        return NULL;
    }
    return tmpfile();
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
    _stub_sleep_count++;
    if (_stub_stop_recording_on_sleep) {
        _SystemView_SetRunning(&_sysview_state, false);
    }
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

int RTTBridge_GetValidatedRTTRegion(uintptr_t *address, size_t *region_size) {
    if (address != NULL) {
        *address = 1u;
    }
    if (region_size != NULL) {
        *region_size = 1u;
    }
    return _stub_rtt_region_result;
}

int RTTBridge_ReadUpBufferNoLock(unsigned channel, void *buffer, size_t buffer_size) {
    size_t copy_size;

    (void)channel;
    _stub_rtt_read_count++;
    if ((_stub_rtt_read_result > 0) && (buffer != NULL) && (buffer_size > 0u)) {
        copy_size = (size_t)_stub_rtt_read_result;
        if (copy_size > buffer_size) {
            copy_size = buffer_size;
        }
        memset(buffer, 'S', copy_size);
    }
    if (_stub_stop_recording_on_read) {
        _SystemView_SetRunning(&_sysview_state, false);
    }
    if ((_stub_stop_recording_after_reads > 0u) &&
        (_stub_rtt_read_count >= _stub_stop_recording_after_reads)) {
        _SystemView_SetRunning(&_sysview_state, false);
    }
    return _stub_rtt_read_result;
}

int RTTBridge_WriteDownBufferNoLock(unsigned channel, const void *buffer, size_t num_bytes) {
    (void)channel;
    (void)buffer;
    return (int)num_bytes;
}

void RTTBridge_IncrementPolls(void) {
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

void _SystemView_ServiceThread(void *pArg) {
    (void)pArg;
}

static int _TestSystemViewHelloHelpers(void) {
    unsigned char message[SYSVIEW_HELLO_SIZE];

    memset(message, 0xff, sizeof(message));
    TEST_ASSERT(SystemView_BuildHelloMessage(message, sizeof(message)));
    TEST_ASSERT(SystemView_HelloHasValidPrefix(message, sizeof(message)));
    TEST_ASSERT(memcmp(message, SYSVIEW_HELLO_PREFIX, SYSVIEW_HELLO_PREFIX_SIZE) == 0);
    TEST_ASSERT(message[SYSVIEW_HELLO_PREFIX_SIZE] == ' ');
    TEST_ASSERT(message[SYSVIEW_HELLO_PREFIX_SIZE + 1u] == 'V');
    TEST_ASSERT(message[SYSVIEW_HELLO_PREFIX_SIZE + 2u] == (unsigned char)('0' + SEGGER_SYSVIEW_MAJOR));
    TEST_ASSERT(message[SYSVIEW_HELLO_PREFIX_SIZE + 3u] == '.');
    TEST_ASSERT(message[SYSVIEW_HELLO_PREFIX_SIZE + 4u] == (unsigned char)('0' + (SEGGER_SYSVIEW_MINOR / 10)));
    TEST_ASSERT(message[SYSVIEW_HELLO_PREFIX_SIZE + 5u] == (unsigned char)('0' + (SEGGER_SYSVIEW_MINOR % 10)));
    TEST_ASSERT(message[SYSVIEW_HELLO_PREFIX_SIZE + 6u] == '.');
    TEST_ASSERT(message[SYSVIEW_HELLO_PREFIX_SIZE + 7u] == (unsigned char)('0' + (SEGGER_SYSVIEW_REV / 10)));
    TEST_ASSERT(message[SYSVIEW_HELLO_PREFIX_SIZE + 8u] == (unsigned char)('0' + (SEGGER_SYSVIEW_REV % 10)));
    TEST_ASSERT(!SystemView_BuildHelloMessage(NULL, sizeof(message)));
    TEST_ASSERT(!SystemView_BuildHelloMessage(message, sizeof(message) - 1u));
    TEST_ASSERT(!SystemView_HelloHasValidPrefix(NULL, sizeof(message)));
    message[0] = 'X';
    TEST_ASSERT(!SystemView_HelloHasValidPrefix(message, sizeof(message)));
    return 0;
}

static int _TestSystemViewRecordFileHeader(void) {
    SystemView_State_t state;
    FILE              *file;
    FILE              *null_state_file;
    char               buffer[512];
    char               expected_version[64];

    memset(&state, 0, sizeof(state));
    state.Config.channel = 2u;

    TEST_ASSERT(_SystemView_WriteRecordFileHeader(&state, NULL) == 0);

    file = tmpfile();
    TEST_ASSERT(file != NULL);
    TEST_ASSERT(_SystemView_WriteRecordFileHeader(&state, file) == 0);
    TEST_ASSERT(Test_ReadTmpFile(file, buffer, sizeof(buffer)) == 0);

    snprintf(expected_version,
             sizeof(expected_version),
             "; Version     SEGGER SystemViewer V%u.%02u.%02u\n",
             SYSVIEW_VERSION_MAJOR,
             SYSVIEW_VERSION_MINOR,
             SYSVIEW_VERSION_REV);
    TEST_ASSERT(buffer[0] == ';');
    TEST_ASSERT(buffer[1] == '\n');
    TEST_ASSERT(strstr(buffer, expected_version) != NULL);
    TEST_ASSERT(strstr(buffer, "; RecordTime  ") != NULL);
    TEST_ASSERT(strstr(buffer, "; Author      CineLogic TraceHub\n") != NULL);
    TEST_ASSERT(strstr(buffer, "; Title       TraceHub SystemView Recording\n") != NULL);
    TEST_ASSERT(strstr(buffer, "; Description TraceHub SystemView RTT recording, channel 2\n") != NULL);
    TEST_ASSERT(strstr(buffer, ";\n\n") != NULL);

    fclose(file);

    null_state_file = tmpfile();
    TEST_ASSERT(null_state_file != NULL);
    TEST_ASSERT(_SystemView_WriteRecordFileHeader(NULL, null_state_file) == 0);
    TEST_ASSERT(Test_ReadTmpFile(null_state_file, buffer, sizeof(buffer)) == 0);
    TEST_ASSERT(strstr(buffer, "; Description TraceHub SystemView RTT recording, channel 0\n") != NULL);
    fclose(null_state_file);
    return 0;
}

static int _TestSystemViewLifecycle(void) {
    SystemView_Config_t config;

    _ResetSystemViewStubs();
    memset(&config, 0, sizeof(config));
    _stub_mutex_init_result = -1;
    TEST_ASSERT(SystemView_Init(&config) == -1);

    _ResetSystemViewStubs();
    SystemView_Stop();
    TEST_ASSERT(SystemView_Start() == -1);
    TEST_ASSERT(SystemView_Init(NULL) == -1);

    memset(&config, 0, sizeof(config));
    config.port = 19111u;
    config.channel = 2u;
    config.enabled = false;
    config.network_enabled = false;
    config.record_enabled = false;
    TEST_ASSERT(SystemView_Init(&config) == 0);
    TEST_ASSERT(!SystemView_IsEnabled());
    TEST_ASSERT(!SystemView_HasFatalError());
    TEST_ASSERT(SystemView_Start() == 0);
    SystemView_Status();
    SystemView_Stop();

    config.enabled = true;
    config.network_enabled = false;
    config.record_enabled = false;
    TEST_ASSERT(SystemView_Init(&config) == 0);
    TEST_ASSERT(SystemView_IsEnabled());
    TEST_ASSERT(SystemView_Start() == -1);
    SystemView_Stop();

    config.enabled = true;
    config.record_enabled = true;
    config.record_prefix = NULL;
    TEST_ASSERT(SystemView_Init(&config) == -1);

    _ResetSystemViewStubs();
    _stub_log_create_file_fail = true;
    config.record_prefix = "tracehub_unit_sysview";
    TEST_ASSERT(SystemView_Init(&config) == -1);

    _ResetSystemViewStubs();
    config.record_prefix = "tracehub_unit_sysview";
    TEST_ASSERT(SystemView_Init(&config) == 0);
    TEST_ASSERT(SystemView_IsEnabled());
    SystemView_Status();
    SystemView_Stop();
    return 0;
}

static int _TestSystemViewRecordStartAndStop(void) {
    SystemView_Config_t config;

    _ResetSystemViewStubs();
    memset(&config, 0, sizeof(config));
    config.port = 19112u;
    config.channel = 2u;
    config.enabled = true;
    config.network_enabled = false;
    config.record_enabled = true;
    config.record_prefix = "tracehub_unit_sysview";
    _stub_thread_results[0] = 0;

    TEST_ASSERT(SystemView_Init(&config) == 0);
    TEST_ASSERT(SystemView_Start() == 0);
    TEST_ASSERT(_stub_thread_call_count == 1u);
    TEST_ASSERT(SystemView_IsEnabled());
    TEST_ASSERT(!SystemView_HasFatalError());
    SystemView_Status();
    SystemView_Stop();
    TEST_ASSERT(_stub_thread_wait_count == 1u);
    TEST_ASSERT(!_sysview_state.Initialized);
    TEST_ASSERT(!_sysview_state.Running);
    TEST_ASSERT(!_sysview_state.LockInitialized);
    return 0;
}

static int _TestSystemViewNetworkStartAndStop(void) {
    SystemView_Config_t config;

    _ResetSystemViewStubs();
    memset(&config, 0, sizeof(config));
    config.port = 19113u;
    config.channel = 2u;
    config.enabled = true;
    config.network_enabled = true;
    config.record_enabled = false;
    _stub_socket_open_result = (SYS_SOCKET_HANDLE)45;
    _stub_socket_listen_result = 0;
    _stub_thread_results[0] = 0;
    _stub_thread_results[1] = 0;

    TEST_ASSERT(SystemView_Init(&config) == 0);
    TEST_ASSERT(SystemView_Start() == 0);
    TEST_ASSERT(_stub_thread_call_count == 2u);
    SystemView_Status();
    SystemView_Stop();
    TEST_ASSERT(_stub_thread_wait_count == 2u);
    TEST_ASSERT(_stub_socket_close_count == 1u);
    TEST_ASSERT(!_sysview_state.Initialized);
    TEST_ASSERT(!_sysview_state.Running);
    TEST_ASSERT(!_sysview_state.LockInitialized);
    return 0;
}

static int _TestSystemViewStartFailureCleanup(void) {
    SystemView_Config_t config;

    _ResetSystemViewStubs();
    memset(&config, 0, sizeof(config));
    config.port = 19114u;
    config.channel = 2u;
    config.enabled = true;
    config.network_enabled = false;
    config.record_enabled = true;
    config.record_prefix = "tracehub_unit_sysview";
    TEST_ASSERT(SystemView_Init(&config) == 0);
    TEST_ASSERT(SystemView_Start() == -1);
    TEST_ASSERT(_stub_thread_call_count == 1u);
    SystemView_Stop();

    _ResetSystemViewStubs();
    memset(&config, 0, sizeof(config));
    config.port = 19115u;
    config.channel = 2u;
    config.enabled = true;
    config.network_enabled = true;
    config.record_enabled = false;
    _stub_thread_results[0] = 0;
    _stub_socket_open_result = SYS_SOCKET_INVALID_HANDLE;
    TEST_ASSERT(SystemView_Init(&config) == 0);
    TEST_ASSERT(SystemView_Start() == -1);
    TEST_ASSERT(_stub_thread_wait_count == 1u);
    SystemView_Stop();

    _ResetSystemViewStubs();
    _stub_thread_results[0] = 0;
    _stub_socket_open_result = (SYS_SOCKET_HANDLE)46;
    _stub_socket_listen_result = -1;
    TEST_ASSERT(SystemView_Init(&config) == 0);
    TEST_ASSERT(SystemView_Start() == -1);
    TEST_ASSERT(_stub_socket_close_count == 1u);
    TEST_ASSERT(_stub_thread_wait_count == 1u);
    SystemView_Stop();

    _ResetSystemViewStubs();
    _stub_thread_results[0] = 0;
    _stub_thread_results[1] = -1;
    _stub_socket_open_result = (SYS_SOCKET_HANDLE)47;
    _stub_socket_listen_result = 0;
    TEST_ASSERT(SystemView_Init(&config) == 0);
    TEST_ASSERT(SystemView_Start() == -1);
    TEST_ASSERT(_stub_thread_call_count == 2u);
    TEST_ASSERT(_stub_socket_close_count == 1u);
    TEST_ASSERT(_stub_thread_wait_count == 1u);
    SystemView_Stop();
    return 0;
}

static int _TestSystemViewRecordingThreadPaths(void) {
    char               queue_buffer[16];
    char               file_buffer[16];
    size_t             len;
    unsigned           flush_reads;

    _ResetSystemViewStubs();
    memset(&_sysview_state, 0, sizeof(_sysview_state));
    _sysview_state.Config.channel = 2u;
    _sysview_state.Config.network_enabled = true;
    _sysview_state.Config.record_enabled = true;
    _sysview_state.LockInitialized = true;
    _sysview_state.Running = true;
    _sysview_state.record_file = tmpfile();
    TEST_ASSERT(_sysview_state.record_file != NULL);
    ByteQueue_Init(&_sysview_state.NetworkQueue, queue_buffer, sizeof(queue_buffer));
    _stub_rtt_read_result = 4;
    _stub_stop_recording_on_read = true;

    _SystemView_RecordingThread(NULL);
    TEST_ASSERT(_stub_rtt_read_count == 1u);
    TEST_ASSERT(ByteQueue_GetUsed(&_sysview_state.NetworkQueue) == 4u);
    rewind(_sysview_state.record_file);
    len = fread(file_buffer, 1u, sizeof(file_buffer), _sysview_state.record_file);
    TEST_ASSERT(!ferror(_sysview_state.record_file));
    TEST_ASSERT(len == 4u);
    TEST_ASSERT(memcmp(file_buffer, "SSSS", 4u) == 0);
    TEST_ASSERT(fclose(_sysview_state.record_file) == 0);
    _sysview_state.record_file = NULL;

    _ResetSystemViewStubs();
    memset(&_sysview_state, 0, sizeof(_sysview_state));
    _sysview_state.Config.channel = 2u;
    _sysview_state.Config.record_enabled = true;
    _sysview_state.LockInitialized = true;
    _sysview_state.Running = true;
    _sysview_state.record_file = tmpfile();
    TEST_ASSERT(_sysview_state.record_file != NULL);
    flush_reads = (SYSVIEW_RECORD_FLUSH_THRESHOLD + SYSVIEW_SEND_BUF_SIZE - 1u) /
                  SYSVIEW_SEND_BUF_SIZE;
    _stub_rtt_read_result = SYSVIEW_SEND_BUF_SIZE;
    _stub_stop_recording_after_reads = flush_reads;

    _SystemView_RecordingThread(NULL);
    TEST_ASSERT(_stub_rtt_read_count == flush_reads);
    TEST_ASSERT(!_sysview_state.FatalError);
    TEST_ASSERT(fclose(_sysview_state.record_file) == 0);
    _sysview_state.record_file = NULL;

    _ResetSystemViewStubs();
    memset(&_sysview_state, 0, sizeof(_sysview_state));
    _sysview_state.Config.channel = 2u;
    _sysview_state.LockInitialized = true;
    _sysview_state.Running = true;
    _stub_rtt_read_result = -1;
    _stub_stop_recording_on_sleep = true;

    _SystemView_RecordingThread(NULL);
    TEST_ASSERT(_stub_rtt_read_count == 1u);
    TEST_ASSERT(_stub_sleep_count == 1u);
    TEST_ASSERT(_sysview_state.RTTRecovering);
    TEST_ASSERT(!_sysview_state.FatalError);

    _ResetSystemViewStubs();
    memset(&_sysview_state, 0, sizeof(_sysview_state));
    _sysview_state.Config.channel = 2u;
    _sysview_state.LockInitialized = true;
    _sysview_state.Running = true;
    _stub_rtt_read_result = 0;
    _stub_stop_recording_on_sleep = true;

    _SystemView_RecordingThread(NULL);
    TEST_ASSERT(_stub_rtt_read_count == 1u);
    TEST_ASSERT(_stub_sleep_count == 1u);
    TEST_ASSERT(!_sysview_state.FatalError);

    _ResetSystemViewStubs();
    memset(&_sysview_state, 0, sizeof(_sysview_state));
    _sysview_state.Config.channel = 2u;
    _sysview_state.LockInitialized = true;
    _sysview_state.Running = true;
    _stub_rtt_region_result = -1;
    _stub_stop_recording_on_sleep = true;

    _SystemView_RecordingThread(NULL);
    TEST_ASSERT(_stub_sleep_count == 1u);
    TEST_ASSERT(_sysview_state.RTTRecovering);
    TEST_ASSERT(!_sysview_state.FatalError);
    return 0;
}

static int _TestSystemViewStateHelpers(void) {
    SystemView_State_t state;
    char               queue_buffer[4];
    char               read_buffer[8];

    memset(&state, 0, sizeof(state));
    state.Config.channel = 2u;
    state.hClient = SYS_SOCKET_INVALID_HANDLE;

    TEST_ASSERT(!_SystemView_IsRunning(NULL));
    _SystemView_SetRunning(NULL, true);
    TEST_ASSERT(!_SystemView_HasFatalError(NULL));
    TEST_ASSERT(_SystemView_GetClient(NULL) == SYS_SOCKET_INVALID_HANDLE);

    _SystemView_SetRunning(&state, true);
    TEST_ASSERT(_SystemView_IsRunning(&state));
    _SystemView_SetRunning(&state, false);
    TEST_ASSERT(!_SystemView_IsRunning(&state));

    _SystemView_ReportFatalServiceError(&state, NULL);
    TEST_ASSERT(_SystemView_HasFatalError(&state));
    TEST_ASSERT(!state.Running);

    memset(&state, 0, sizeof(state));
    state.Config.channel = 2u;
    state.Config.network_enabled = true;
    state.Config.record_enabled = true;
    ByteQueue_Init(&state.NetworkQueue, queue_buffer, sizeof(queue_buffer));
    TEST_ASSERT(ByteQueue_Write(&state.NetworkQueue, "abcd", 4u) == BYTE_QUEUE_WRITE_OK);
    _SystemView_RequestNetworkStreamResetLocked(&state);
    TEST_ASSERT(state.ClientDisconnectRequested);
    TEST_ASSERT(ByteQueue_GetUsed(&state.NetworkQueue) == 0u);

    state.SendNumBytes = 2;
    state.WriteNumBytes = 3;
    state.AppPacketExpected = 4u;
    state.AppPacketReceived = 1u;
    state.pSendBuf = state.acSendBuf;
    state.pWriteBuf = state.acWriteBuf;
    TEST_ASSERT(_SystemView_SetConnectedClient(&state, (SYS_SOCKET_HANDLE)9) == 1u);
    TEST_ASSERT(state.hClient == (SYS_SOCKET_HANDLE)9);
    TEST_ASSERT(state.HandshakeDone);
    TEST_ASSERT(!state.ClientDisconnectRequested);
    TEST_ASSERT(state.SendNumBytes == 0);
    TEST_ASSERT(state.WriteNumBytes == 0);
    TEST_ASSERT(state.AppPacketExpected == 0u);
    TEST_ASSERT(state.AppPacketReceived == 0u);
    TEST_ASSERT(state.pSendBuf == NULL);
    TEST_ASSERT(state.pWriteBuf == NULL);

    _SystemView_AddBytesSent(&state, 7u);
    _SystemView_AddBytesReceived(&state, 8u);
    TEST_ASSERT(state.BytesSent == 7u);
    TEST_ASSERT(state.BytesReceived == 8u);

    TEST_ASSERT(_SystemView_QueueTargetData(&state, "abc", 3u) == 0);
    TEST_ASSERT(_SystemView_DequeueTargetData(&state, read_buffer, sizeof(read_buffer)) == 3u);
    TEST_ASSERT(memcmp(read_buffer, "abc", 3u) == 0);
    TEST_ASSERT(_SystemView_QueueTargetData(&state, "abcd", 4u) == 0);
    TEST_ASSERT(_SystemView_QueueTargetData(&state, "xy", 2u) == 0);
    TEST_ASSERT(state.ClientDisconnectRequested);
    TEST_ASSERT(_SystemView_TakeClientDisconnectRequest(&state));
    TEST_ASSERT(!state.ClientDisconnectRequested);
    TEST_ASSERT(_SystemView_QueueTargetData(&state, "abcdef", 6u) == -1);
    TEST_ASSERT(state.FatalError);

    memset(&state, 0, sizeof(state));
    state.Config.channel = 2u;
    _SystemView_ReportRecordFileError(&state, NULL, 0);
    TEST_ASSERT(state.RecordFileError);
    TEST_ASSERT(state.FatalError);
    TEST_ASSERT(_SystemView_FlushRecordFile(&state, NULL) == 0);
    return 0;
}

static int _TestSystemViewRecoveryAndChannelHelpers(void) {
    SystemView_State_t state;
    char               queue_buffer[4];
    char               read_buffer[8];

    _ResetSystemViewStubs();
    memset(&state, 0, sizeof(state));
    state.Config.channel = 2u;
    state.hClient = (SYS_SOCKET_HANDLE)5;
    TEST_ASSERT(_SystemView_GetClient(&state) == (SYS_SOCKET_HANDLE)5);
    state.LockInitialized = true;
    state.hClient = (SYS_SOCKET_HANDLE)6;
    TEST_ASSERT(_SystemView_GetClient(&state) == (SYS_SOCKET_HANDLE)6);
    _SystemView_SetRunning(&state, true);
    TEST_ASSERT(_SystemView_IsRunning(&state));
    _SystemView_ReportFatalServiceError(&state, "locked");
    TEST_ASSERT(_SystemView_HasFatalError(&state));
    TEST_ASSERT(!state.Running);
    _SystemView_ReportFatalServiceError(&state, "again");
    TEST_ASSERT(!state.Running);

    memset(&state, 0, sizeof(state));
    state.Config.channel = 2u;
    state.Config.network_enabled = true;
    state.LockInitialized = true;
    ByteQueue_Init(&state.NetworkQueue, queue_buffer, sizeof(queue_buffer));
    TEST_ASSERT(ByteQueue_Write(&state.NetworkQueue, "abcd", 4u) == BYTE_QUEUE_WRITE_OK);
    _SystemView_ReportRecoverableRTTError(&state, NULL);
    TEST_ASSERT(state.RTTRecovering);
    TEST_ASSERT(state.ClientDisconnectRequested);
    TEST_ASSERT(ByteQueue_GetUsed(&state.NetworkQueue) == 0u);
    TEST_ASSERT(!state.FatalError);
    _SystemView_ReportRecoverableRTTError(&state, "read");
    TEST_ASSERT(state.RTTRecovering);
    _SystemView_ReportRTTRecovered(&state);
    TEST_ASSERT(!state.RTTRecovering);
    _SystemView_ReportRTTRecovered(&state);
    _SystemView_ReportRecoverableRTTError(NULL, NULL);
    _SystemView_ReportRTTRecovered(NULL);

    memset(&state, 0, sizeof(state));
    state.Config.channel = 2u;
    state.Config.network_enabled = false;
    ByteQueue_Init(&state.NetworkQueue, queue_buffer, sizeof(queue_buffer));
    TEST_ASSERT(ByteQueue_Write(&state.NetworkQueue, "abcd", 4u) == BYTE_QUEUE_WRITE_OK);
    _SystemView_RequestNetworkStreamResetLocked(NULL);
    _SystemView_RequestNetworkStreamResetLocked(&state);
    TEST_ASSERT(!state.ClientDisconnectRequested);
    TEST_ASSERT(ByteQueue_GetUsed(&state.NetworkQueue) == 4u);

    TEST_ASSERT(_SystemView_QueueTargetData(NULL, "abc", 3u) == 0);
    TEST_ASSERT(_SystemView_QueueTargetData(&state, NULL, 3u) == 0);
    TEST_ASSERT(_SystemView_QueueTargetData(&state, "abc", 0u) == 0);
    TEST_ASSERT(_SystemView_QueueTargetData(&state, "abc", 3u) == 0);
    TEST_ASSERT(_SystemView_DequeueTargetData(NULL, read_buffer, sizeof(read_buffer)) == 0u);
    TEST_ASSERT(_SystemView_DequeueTargetData(&state, NULL, sizeof(read_buffer)) == 0u);
    TEST_ASSERT(_SystemView_DequeueTargetData(&state, read_buffer, 0u) == 0u);

    memset(&state, 0, sizeof(state));
    state.Config.channel = 2u;
    state.Config.network_enabled = true;
    TEST_ASSERT(_SystemView_QueueTargetData(&state, "abc", 3u) == -1);
    TEST_ASSERT(state.FatalError);
    TEST_ASSERT(_SystemView_DequeueTargetData(&state, read_buffer, sizeof(read_buffer)) == 0u);

    _ResetSystemViewStubs();
    memset(&state, 0, sizeof(state));
    state.Config.channel = 2u;
    state.Config.network_enabled = true;
    TEST_ASSERT(_SystemView_CheckChannels(NULL) == -1);
    _stub_rtt_up_check_result = -1;
    TEST_ASSERT(_SystemView_CheckChannels(&state) == -1);
    TEST_ASSERT(_stub_rtt_down_check_count == 0u);

    _ResetSystemViewStubs();
    _stub_rtt_down_check_result = -1;
    TEST_ASSERT(_SystemView_CheckChannels(&state) == -1);

    _ResetSystemViewStubs();
    state.Config.network_enabled = false;
    _stub_rtt_down_check_result = -1;
    TEST_ASSERT(_SystemView_CheckChannels(&state) == 0);
    TEST_ASSERT(_stub_rtt_down_check_count == 0u);

    _ResetSystemViewStubs();
    state.Config.network_enabled = true;
    TEST_ASSERT(_SystemView_CheckChannels(&state) == 0);
    TEST_ASSERT(_stub_rtt_up_check_count == 1u);
    TEST_ASSERT(_stub_rtt_down_check_count == 1u);

    memset(&state, 0, sizeof(state));
    state.Config.network_enabled = true;
    state.hClient = (SYS_SOCKET_HANDLE)8;
    state.HandshakeDone = true;
    state.ClientDisconnectRequested = true;
    ByteQueue_Init(&state.NetworkQueue, queue_buffer, sizeof(queue_buffer));
    TEST_ASSERT(ByteQueue_Write(&state.NetworkQueue, "abcd", 4u) == BYTE_QUEUE_WRITE_OK);
    _SystemView_CloseClient(NULL);
    _SystemView_CloseClient(&state);
    TEST_ASSERT(state.hClient == SYS_SOCKET_INVALID_HANDLE);
    TEST_ASSERT(!state.HandshakeDone);
    TEST_ASSERT(!state.ClientDisconnectRequested);
    TEST_ASSERT(ByteQueue_GetUsed(&state.NetworkQueue) == 0u);

    state.hClient = (SYS_SOCKET_HANDLE)9;
    state.HandshakeDone = true;
    _SystemView_CloseClientForNetworkDisconnect(&state, NULL);
    TEST_ASSERT(state.hClient == SYS_SOCKET_INVALID_HANDLE);
    state.hClient = (SYS_SOCKET_HANDLE)10;
    state.HandshakeDone = true;
    _SystemView_CloseClientForNetworkError(&state, NULL);
    TEST_ASSERT(state.hClient == SYS_SOCKET_INVALID_HANDLE);
    TEST_ASSERT(_stub_socket_shutdown_count == 3u);
    TEST_ASSERT(_stub_socket_close_count == 3u);

    memset(&state, 0, sizeof(state));
    state.Config.channel = 2u;
    state.Running = true;
    state.LockInitialized = true;
    _SystemView_ReportRecordFileError(NULL, NULL, 0);
    _SystemView_ReportRecordFileError(&state, "flush", ERANGE);
    TEST_ASSERT(state.RecordFileError);
    TEST_ASSERT(state.FatalError);
    TEST_ASSERT(!state.Running);
    state.Running = true;
    _SystemView_ReportRecordFileError(&state, "again", 0);
    TEST_ASSERT(!state.Running);

    _ResetSystemViewStubs();
    return 0;
}

int main(void) {
    TEST_RUN(_TestSystemViewHelloHelpers);
    TEST_RUN(_TestSystemViewRecordFileHeader);
    TEST_RUN(_TestSystemViewLifecycle);
    TEST_RUN(_TestSystemViewRecordStartAndStop);
    TEST_RUN(_TestSystemViewNetworkStartAndStop);
    TEST_RUN(_TestSystemViewStartFailureCleanup);
    TEST_RUN(_TestSystemViewRecordingThreadPaths);
    TEST_RUN(_TestSystemViewStateHelpers);
    TEST_RUN(_TestSystemViewRecoveryAndChannelHelpers);
    return 0;
}

/*************************** End of file ****************************/
