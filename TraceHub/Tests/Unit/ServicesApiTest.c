/*********************************************************************
*                             CineLogic                              *
*                      RTT Trace and Debug Bridge                    *
**********************************************************************
----------------------------------------------------------------------
File    : ServicesApiTest.c
Purpose : Unit checks for Terminal and SystemView public APIs
---------------------------END-OF-HEADER------------------------------
*/

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "Terminal_internal.h"
#include "SystemView_internal.h"
#include "CoreLogRecorder.h"
#include "RTTBridge.h"
#include "TestCommon.h"

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

FILE *LOG_CreateTimestampedFileEx(const char *prefix, const char *extension, const char *mode) {
    (void)prefix;
    (void)extension;
    (void)mode;
    return tmpfile();
}

int SYS_MutexInit(SYS_Mutex *mutex) {
    (void)mutex;
    return 0;
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
    (void)threadEntry;
    (void)context;
    (void)pRetTid;
    return -1;
}

void SYS_WaitThreadTerm(SYS_Thread pRetTid) {
    (void)pRetTid;
}

unsigned SYS_GetTickCount(void) {
    return 0u;
}

void SYS_Sleep(unsigned ms) {
    (void)ms;
}

bool CoreLogRecorder_IsCoreChannel(unsigned channel) {
    (void)channel;
    return false;
}

int CoreLogRecorder_RegisterConsumer(unsigned channel) {
    (void)channel;
    return -1;
}

void CoreLogRecorder_UnregisterConsumer(unsigned channel) {
    (void)channel;
}

int CoreLogRecorder_ReadChannel(unsigned channel, void *buffer, size_t buffer_size) {
    (void)channel;
    (void)buffer;
    (void)buffer_size;
    return -1;
}

int RTTBridge_CheckUpBufferChannel(unsigned channel) {
    (void)channel;
    return 0;
}

int RTTBridge_CheckDownBufferChannel(unsigned channel) {
    (void)channel;
    return 0;
}

int RTTBridge_GetValidatedRTTRegion(uintptr_t *address, size_t *region_size) {
    if (address != NULL) {
        *address = 1u;
    }
    if (region_size != NULL) {
        *region_size = 1u;
    }
    return 0;
}

int RTTBridge_ReadUpBufferNoLock(unsigned channel, void *buffer, size_t buffer_size) {
    (void)channel;
    (void)buffer;
    (void)buffer_size;
    return 0;
}

int RTTBridge_WriteDownBufferNoLock(unsigned channel, const void *buffer, size_t num_bytes) {
    (void)channel;
    (void)buffer;
    return (int)num_bytes;
}

void RTTBridge_IncrementPolls(void) {
}

SYS_SOCKET_HANDLE SYS_SOCKET_OpenTCP(void) {
    return SYS_SOCKET_INVALID_HANDLE;
}

void SYS_SOCKET_Close(SYS_SOCKET_HANDLE hSocket) {
    (void)hSocket;
}

int SYS_SOCKET_ListenAtTCPAddr(SYS_SOCKET_HANDLE hSocket,
                               unsigned IPAddr,
                               unsigned Port,
                               unsigned NumConnectionsQueued) {
    (void)hSocket;
    (void)IPAddr;
    (void)Port;
    (void)NumConnectionsQueued;
    return -1;
}

void TelnetCodec_Reset(TelnetCodec_State_t *state) {
    if (state != NULL) {
        state->State = TELNET_CODEC_STATE_DATA;
        state->Command = 0u;
    }
}

int _Console_SetRawMode(void) {
    return -1;
}

void _Console_RestoreMode(void) {
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

void _SystemView_ServiceThread(void *pArg) {
    (void)pArg;
}

static int _TestTerminalLifecycle(void) {
    Terminal_Config_t config;

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

static int _TestSystemViewHelloHelpers(void) {
    unsigned char message[SYSVIEW_HELLO_SIZE];

    memset(message, 0xff, sizeof(message));
    TEST_ASSERT(SystemView_BuildHelloMessage(message, sizeof(message)));
    TEST_ASSERT(SystemView_HelloHasValidPrefix(message, sizeof(message)));
    TEST_ASSERT(memcmp(message, SYSVIEW_HELLO_PREFIX, SYSVIEW_HELLO_PREFIX_SIZE) == 0);
    TEST_ASSERT(!SystemView_BuildHelloMessage(NULL, sizeof(message)));
    TEST_ASSERT(!SystemView_BuildHelloMessage(message, sizeof(message) - 1u));
    TEST_ASSERT(!SystemView_HelloHasValidPrefix(NULL, sizeof(message)));
    message[0] = 'X';
    TEST_ASSERT(!SystemView_HelloHasValidPrefix(message, sizeof(message)));
    return 0;
}

static int _TestSystemViewLifecycle(void) {
    SystemView_Config_t config;

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

    config.record_prefix = "tracehub_unit_sysview";
    TEST_ASSERT(SystemView_Init(&config) == 0);
    TEST_ASSERT(SystemView_IsEnabled());
    SystemView_Status();
    SystemView_Stop();
    return 0;
}

int main(void) {
    TEST_RUN(_TestTerminalLifecycle);
    TEST_RUN(_TestSystemViewHelloHelpers);
    TEST_RUN(_TestSystemViewLifecycle);
    return 0;
}

/*************************** End of file ****************************/
