/*********************************************************************
*                             CineLogic                              *
*                      RTT Trace and Debug Bridge                    *
**********************************************************************
----------------------------------------------------------------------
File    : NetworkApiTest.c
Purpose : Unit checks for network and Telnet public APIs
---------------------------END-OF-HEADER------------------------------
*/

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "Socket.h"
#include "TelnetCodec.h"
#include "TestCommon.h"

#define TELNET_IAC_BYTE   ((char)0xFF)
#define TELNET_SE_BYTE    ((char)0xF0)
#define TELNET_SB_BYTE    ((char)0xFA)
#define TELNET_WILL_BYTE  ((char)0xFB)
#define TELNET_DO_BYTE    ((char)0xFD)
#define TELNET_DONT_BYTE  ((char)0xFE)
#define TELOPT_ECHO_BYTE  ((char)0x01)
#define TELOPT_SGA_BYTE   ((char)0x03)
#define TELOPT_NAWS_BYTE  ((char)0x1F)

void LOG_Warn(const char *sFormat, ...) {
    (void)sFormat;
}

static int _TestTelnetResetAndPlainData(void) {
    TelnetCodec_State_t state;
    char                output[8];
    char                response[8];
    unsigned            response_len;
    unsigned            output_len;

    memset(&state, 0xff, sizeof(state));
    TelnetCodec_Reset(&state);
    TEST_ASSERT(state.State == TELNET_CODEC_STATE_DATA);
    TEST_ASSERT(state.Command == 0u);
    TelnetCodec_Reset(NULL);

    response_len = 99u;
    output_len = TelnetCodec_FilterClientData(&state,
                                              "ab",
                                              2u,
                                              output,
                                              sizeof(output),
                                              response,
                                              sizeof(response),
                                              &response_len);
    TEST_ASSERT(output_len == 2u);
    TEST_ASSERT(memcmp(output, "ab", 2u) == 0);
    TEST_ASSERT(response_len == 0u);

    output_len = TelnetCodec_FilterClientData(NULL,
                                              "ab",
                                              2u,
                                              output,
                                              sizeof(output),
                                              response,
                                              sizeof(response),
                                              &response_len);
    TEST_ASSERT(output_len == 0u);
    TEST_ASSERT(response_len == 0u);
    return 0;
}

static int _TestTelnetCommandFiltering(void) {
    TelnetCodec_State_t state;
    char                input[] = {
        'A',
        TELNET_IAC_BYTE, TELNET_IAC_BYTE,
        TELNET_IAC_BYTE, TELNET_DO_BYTE, TELOPT_ECHO_BYTE,
        TELNET_IAC_BYTE, TELNET_SB_BYTE, TELOPT_NAWS_BYTE, 0x00, 0x50,
        TELNET_IAC_BYTE, TELNET_SE_BYTE,
        'B'
    };
    char                output[8];
    char                response[8];
    unsigned            response_len;
    unsigned            output_len;

    TelnetCodec_Reset(&state);
    output_len = TelnetCodec_FilterClientData(&state,
                                              input,
                                              (unsigned)sizeof(input),
                                              output,
                                              sizeof(output),
                                              response,
                                              sizeof(response),
                                              &response_len);
    TEST_ASSERT(output_len == 3u);
    TEST_ASSERT(output[0] == 'A');
    TEST_ASSERT((unsigned char)output[1] == 0xFFu);
    TEST_ASSERT(output[2] == 'B');
    TEST_ASSERT(response_len == 3u);
    TEST_ASSERT((unsigned char)response[0] == 0xFFu);
    TEST_ASSERT(response[1] == TELNET_WILL_BYTE);
    TEST_ASSERT(response[2] == TELOPT_ECHO_BYTE);
    return 0;
}

static int _TestTelnetSplitOptionAndResponseCapacity(void) {
    TelnetCodec_State_t state;
    char                part_one[] = { TELNET_IAC_BYTE, TELNET_DO_BYTE };
    char                part_two[] = { TELOPT_SGA_BYTE, 'x' };
    char                output[8];
    char                response[8];
    unsigned            response_len;
    unsigned            output_len;

    TelnetCodec_Reset(&state);
    output_len = TelnetCodec_FilterClientData(&state,
                                              part_one,
                                              sizeof(part_one),
                                              output,
                                              sizeof(output),
                                              response,
                                              sizeof(response),
                                              &response_len);
    TEST_ASSERT(output_len == 0u);
    TEST_ASSERT(response_len == 0u);
    TEST_ASSERT(state.State == TELNET_CODEC_STATE_OPTION);

    output_len = TelnetCodec_FilterClientData(&state,
                                              part_two,
                                              sizeof(part_two),
                                              output,
                                              sizeof(output),
                                              response,
                                              sizeof(response),
                                              &response_len);
    TEST_ASSERT(output_len == 1u);
    TEST_ASSERT(output[0] == 'x');
    TEST_ASSERT(response_len == 3u);
    TEST_ASSERT((unsigned char)response[0] == 0xFFu);
    TEST_ASSERT(response[1] == TELNET_WILL_BYTE);
    TEST_ASSERT(response[2] == TELOPT_SGA_BYTE);

    TelnetCodec_Reset(&state);
    output_len = TelnetCodec_FilterClientData(&state,
                                              part_one,
                                              sizeof(part_one),
                                              output,
                                              sizeof(output),
                                              response,
                                              2u,
                                              &response_len);
    TEST_ASSERT(output_len == 0u);
    TEST_ASSERT(response_len == 0u);

    output_len = TelnetCodec_FilterClientData(&state,
                                              part_two,
                                              1u,
                                              output,
                                              sizeof(output),
                                              response,
                                              2u,
                                              &response_len);
    TEST_ASSERT(output_len == 0u);
    TEST_ASSERT(response_len == 0u);
    TEST_ASSERT(state.State == TELNET_CODEC_STATE_DATA);
    return 0;
}

static int _TestTelnetNegotiationInvalidSocket(void) {
    TEST_ASSERT(TelnetCodec_SendNegotiation(SYS_SOCKET_INVALID_HANDLE) == -1);
    return 0;
}

static int _TestSocketInvalidHandleContracts(void) {
    SYS_SOCKET_HANDLE client;
    char              buffer[4];

    client = (SYS_SOCKET_HANDLE)123;
    TEST_ASSERT(!SYS_SOCKET_IsValidHandle(SYS_SOCKET_INVALID_HANDLE));
    TEST_ASSERT(SYS_SOCKET_IsReadable(SYS_SOCKET_INVALID_HANDLE, 0) == -1);
    TEST_ASSERT(SYS_SOCKET_IsReadable(SYS_SOCKET_INVALID_HANDLE, -1) == -1);
    TEST_ASSERT(SYS_SOCKET_IsWriteable(SYS_SOCKET_INVALID_HANDLE, 0) == -1);
    TEST_ASSERT(SYS_SOCKET_AcceptEx(SYS_SOCKET_INVALID_HANDLE, 0, &client) ==
                SYS_SOCKET_ERR_UNSPECIFIED);
    TEST_ASSERT(client == SYS_SOCKET_INVALID_HANDLE);
    TEST_ASSERT(SYS_SOCKET_AcceptEx(SYS_SOCKET_INVALID_HANDLE, 0, NULL) ==
                SYS_SOCKET_ERR_UNSPECIFIED);
    TEST_ASSERT(SYS_SOCKET_ListenAtTCPAddr(SYS_SOCKET_INVALID_HANDLE,
                                           SYS_SOCKET_IP_ADDR_ANY,
                                           0u,
                                           1u) == -1);
    TEST_ASSERT(SYS_SOCKET_IsReady(SYS_SOCKET_INVALID_HANDLE) ==
                SYS_SOCKET_ERR_UNSPECIFIED);
    TEST_ASSERT(SYS_SOCKET_Send(SYS_SOCKET_INVALID_HANDLE, buffer, sizeof(buffer)) ==
                SYS_SOCKET_ERR_UNSPECIFIED);
    TEST_ASSERT(SYS_SOCKET_SendAll(SYS_SOCKET_INVALID_HANDLE, buffer, sizeof(buffer), 0) ==
                SYS_SOCKET_ERR_UNSPECIFIED);
    TEST_ASSERT(SYS_SOCKET_Receive(SYS_SOCKET_INVALID_HANDLE, buffer, sizeof(buffer)) ==
                SYS_SOCKET_ERR_UNSPECIFIED);

    SYS_SOCKET_Shutdown(SYS_SOCKET_INVALID_HANDLE, SYS_SOCKET_SHUT_RDWR);
    SYS_SOCKET_EnableKeepalive(SYS_SOCKET_INVALID_HANDLE);
    SYS_SOCKET_SetNonBlocking(SYS_SOCKET_INVALID_HANDLE);
    SYS_SOCKET_Close(SYS_SOCKET_INVALID_HANDLE);
    return 0;
}

static int _TestSocketOpenAndClose(void) {
    SYS_SOCKET_HANDLE socket_handle;
    SYS_SOCKET_HANDLE client;

    socket_handle = SYS_SOCKET_OpenTCP();
    client = SYS_SOCKET_INVALID_HANDLE;
    TEST_ASSERT(SYS_SOCKET_IsValidHandle(socket_handle));
    TEST_ASSERT(SYS_SOCKET_IsReady(socket_handle) == 1);
    SYS_SOCKET_EnableKeepalive(socket_handle);
    TEST_ASSERT(SYS_SOCKET_ListenAtTCPAddr(socket_handle,
                                           SYS_SOCKET_IP_ADDR_ANY,
                                           0u,
                                           1u) == 0);
    TEST_ASSERT(SYS_SOCKET_AcceptEx(socket_handle, 0, &client) ==
                SYS_SOCKET_ERR_ACCEPT_TIMEOUT);
    TEST_ASSERT(client == SYS_SOCKET_INVALID_HANDLE);
    SYS_SOCKET_SetNonBlocking(socket_handle);
    SYS_SOCKET_Close(socket_handle);
    return 0;
}

int main(void) {
    TEST_RUN(_TestTelnetResetAndPlainData);
    TEST_RUN(_TestTelnetCommandFiltering);
    TEST_RUN(_TestTelnetSplitOptionAndResponseCapacity);
    TEST_RUN(_TestTelnetNegotiationInvalidSocket);
    TEST_RUN(_TestSocketInvalidHandleContracts);
    TEST_RUN(_TestSocketOpenAndClose);
    return 0;
}

/*************************** End of file ****************************/
