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
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#if !defined(_WIN32)
  #include <arpa/inet.h>
  #include <netinet/in.h>
  #include <sys/socket.h>
#endif

#include "Socket.h"
#include "TelnetCodec.h"
#include "TestCommon.h"

#define TELNET_IAC_BYTE   ((char)0xFF)
#define TELNET_SE_BYTE    ((char)0xF0)
#define TELNET_SB_BYTE    ((char)0xFA)
#define TELNET_WILL_BYTE  ((char)0xFB)
#define TELNET_WONT_BYTE  ((char)0xFC)
#define TELNET_DO_BYTE    ((char)0xFD)
#define TELNET_DONT_BYTE  ((char)0xFE)
#define TELOPT_ECHO_BYTE  ((char)0x01)
#define TELOPT_SGA_BYTE   ((char)0x03)
#define TELOPT_NAWS_BYTE  ((char)0x1F)

void LOG_Warn(const char *sFormat, ...) {
    (void)sFormat;
}

static int _GetSocketPort(SYS_SOCKET_HANDLE socket_handle, uint16_t *port) {
    struct sockaddr_in address;
#if defined(_WIN32)
    int                address_len;
#else
    socklen_t          address_len;
#endif

    if (port == NULL) {
        return -1;
    }

    memset(&address, 0, sizeof(address));
#if defined(_WIN32)
    address_len = (int)sizeof(address);
    if (getsockname((SOCKET)socket_handle,
                    (struct sockaddr *)&address,
                    &address_len) != 0) {
        return -1;
    }
#else
    address_len = (socklen_t)sizeof(address);
    if (getsockname((int)socket_handle,
                    (struct sockaddr *)&address,
                    &address_len) != 0) {
        return -1;
    }
#endif

    *port = ntohs(address.sin_port);
    return (*port == 0u) ? -1 : 0;
}

static int _ConnectSocketToLoopback(SYS_SOCKET_HANDLE socket_handle, uint16_t port) {
    struct sockaddr_in address;

    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = htonl(0x7F000001u);

#if defined(_WIN32)
    return connect((SOCKET)socket_handle,
                   (const struct sockaddr *)&address,
                   (int)sizeof(address));
#else
    return connect((int)socket_handle,
                   (const struct sockaddr *)&address,
                   (socklen_t)sizeof(address));
#endif
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

static int _TestTelnetOptionResponseVariants(void) {
    TelnetCodec_State_t state;
    char                input[] = {
        TELNET_IAC_BYTE, TELNET_DONT_BYTE, TELOPT_ECHO_BYTE,
        TELNET_IAC_BYTE, TELNET_WILL_BYTE, TELOPT_SGA_BYTE,
        TELNET_IAC_BYTE, TELNET_WILL_BYTE, TELOPT_ECHO_BYTE,
        TELNET_IAC_BYTE, TELNET_WONT_BYTE, TELOPT_SGA_BYTE,
        TELNET_IAC_BYTE, 'X'
    };
    char                output[4];
    char                response[16];
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
    TEST_ASSERT(output_len == 0u);
    TEST_ASSERT(response_len == 12u);
    TEST_ASSERT((unsigned char)response[0] == 0xFFu);
    TEST_ASSERT(response[1] == TELNET_WONT_BYTE);
    TEST_ASSERT(response[2] == TELOPT_ECHO_BYTE);
    TEST_ASSERT((unsigned char)response[3] == 0xFFu);
    TEST_ASSERT(response[4] == TELNET_DO_BYTE);
    TEST_ASSERT(response[5] == TELOPT_SGA_BYTE);
    TEST_ASSERT((unsigned char)response[6] == 0xFFu);
    TEST_ASSERT(response[7] == TELNET_DONT_BYTE);
    TEST_ASSERT(response[8] == TELOPT_ECHO_BYTE);
    TEST_ASSERT((unsigned char)response[9] == 0xFFu);
    TEST_ASSERT(response[10] == TELNET_DONT_BYTE);
    TEST_ASSERT(response[11] == TELOPT_SGA_BYTE);
    TEST_ASSERT(state.State == TELNET_CODEC_STATE_DATA);

    state.State = TELNET_CODEC_STATE_OPTION;
    state.Command = 0x42u;
    output_len = TelnetCodec_FilterClientData(&state,
                                              "z",
                                              1u,
                                              output,
                                              sizeof(output),
                                              response,
                                              sizeof(response),
                                              &response_len);
    TEST_ASSERT(output_len == 0u);
    TEST_ASSERT(response_len == 0u);
    TEST_ASSERT(state.State == TELNET_CODEC_STATE_DATA);
    TEST_ASSERT(state.Command == 0u);
    return 0;
}

static int _TestTelnetSubnegotiationIacRecovery(void) {
    TelnetCodec_State_t state;
    char                input[] = {
        TELNET_IAC_BYTE, TELNET_SB_BYTE, TELOPT_NAWS_BYTE,
        TELNET_IAC_BYTE, 'X',
        TELNET_IAC_BYTE, TELNET_SE_BYTE,
        'Z'
    };
    char                output[4];
    char                response[4];
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
    TEST_ASSERT(output_len == 1u);
    TEST_ASSERT(output[0] == 'Z');
    TEST_ASSERT(response_len == 0u);
    TEST_ASSERT(state.State == TELNET_CODEC_STATE_DATA);
    return 0;
}

static int _TestTelnetNegotiationInvalidSocket(void) {
    TEST_ASSERT(TelnetCodec_SendNegotiation(SYS_SOCKET_INVALID_HANDLE) == -1);
    return 0;
}

static int _TestTelnetNegotiationSocketPair(void) {
#if !defined(_WIN32)
    int                 sockets[2];
    char                buffer[16];
    int                 received;
    static const char   expected[] = {
        TELNET_IAC_BYTE, TELNET_WILL_BYTE, TELOPT_ECHO_BYTE,
        TELNET_IAC_BYTE, TELNET_WILL_BYTE, TELOPT_SGA_BYTE,
        TELNET_IAC_BYTE, TELNET_DO_BYTE,   TELOPT_SGA_BYTE,
        TELNET_IAC_BYTE, TELNET_WONT_BYTE, TELOPT_NAWS_BYTE
    };

    TEST_ASSERT(socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == 0);
    TEST_ASSERT(TelnetCodec_SendNegotiation((SYS_SOCKET_HANDLE)sockets[0]) == 0);
    TEST_ASSERT(SYS_SOCKET_IsReadable((SYS_SOCKET_HANDLE)sockets[1], 1000) == 1);
    received = SYS_SOCKET_Receive((SYS_SOCKET_HANDLE)sockets[1],
                                  buffer,
                                  sizeof(buffer));
    SYS_SOCKET_Close((SYS_SOCKET_HANDLE)sockets[0]);
    SYS_SOCKET_Close((SYS_SOCKET_HANDLE)sockets[1]);
    TEST_ASSERT(received == (int)sizeof(expected));
    TEST_ASSERT(memcmp(buffer, expected, sizeof(expected)) == 0);
#endif
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
    char              buffer[4] = {0};

    socket_handle = SYS_SOCKET_OpenTCP();
    client = SYS_SOCKET_INVALID_HANDLE;
    TEST_ASSERT(SYS_SOCKET_IsValidHandle(socket_handle));
    TEST_ASSERT(SYS_SOCKET_IsReady(socket_handle) == 1);
    SYS_SOCKET_EnableKeepalive(socket_handle);
    TEST_ASSERT(SYS_SOCKET_ListenAtTCPAddr(socket_handle,
                                           SYS_SOCKET_IP_ADDR_ANY,
                                           0u,
                                           (unsigned)INT_MAX + 1u) == -1);
    TEST_ASSERT(SYS_SOCKET_ListenAtTCPAddr(socket_handle,
                                           SYS_SOCKET_IP_ADDR_ANY,
                                           0u,
                                           1u) == 0);
    TEST_ASSERT(SYS_SOCKET_AcceptEx(socket_handle, 0, NULL) ==
                SYS_SOCKET_ERR_UNSPECIFIED);
    TEST_ASSERT(SYS_SOCKET_AcceptEx(socket_handle, -1, &client) ==
                SYS_SOCKET_ERR_UNSPECIFIED);
    TEST_ASSERT(client == SYS_SOCKET_INVALID_HANDLE);
    TEST_ASSERT(SYS_SOCKET_AcceptEx(socket_handle, 0, &client) ==
                SYS_SOCKET_ERR_ACCEPT_TIMEOUT);
    TEST_ASSERT(client == SYS_SOCKET_INVALID_HANDLE);
    SYS_SOCKET_SetNonBlocking(socket_handle);
    SYS_SOCKET_Close(socket_handle);
    TEST_ASSERT(SYS_SOCKET_IsReady(socket_handle) == SYS_SOCKET_ERR_UNSPECIFIED);
    TEST_ASSERT(SYS_SOCKET_Send(socket_handle, buffer, sizeof(buffer)) ==
                SYS_SOCKET_ERR_UNSPECIFIED);
    TEST_ASSERT(SYS_SOCKET_SendAll(socket_handle, buffer, sizeof(buffer), 1) ==
                SYS_SOCKET_ERR_UNSPECIFIED);
    TEST_ASSERT(SYS_SOCKET_Receive(socket_handle, buffer, sizeof(buffer)) ==
                SYS_SOCKET_ERR_UNSPECIFIED);
    return 0;
}

static int _TestSocketLoopbackDataFlow(void) {
    SYS_SOCKET_HANDLE listener;
    SYS_SOCKET_HANDLE client;
    SYS_SOCKET_HANDLE accepted;
    uint16_t          port;
    char              buffer[8];
    static const char client_payload[] = "ping";
    static const char server_payload[] = "ok";

    listener = SYS_SOCKET_OpenTCP();
    client = SYS_SOCKET_INVALID_HANDLE;
    accepted = SYS_SOCKET_INVALID_HANDLE;
    TEST_ASSERT(SYS_SOCKET_IsValidHandle(listener));
    TEST_ASSERT(SYS_SOCKET_ListenAtTCPAddr(listener, 0x7F000001u, 0u, 1u) == 0);
    TEST_ASSERT(_GetSocketPort(listener, &port) == 0);

    client = SYS_SOCKET_OpenTCP();
    TEST_ASSERT(SYS_SOCKET_IsValidHandle(client));
    TEST_ASSERT(_ConnectSocketToLoopback(client, port) == 0);
    TEST_ASSERT(SYS_SOCKET_AcceptEx(listener, 1000, &accepted) == 0);
    TEST_ASSERT(SYS_SOCKET_IsValidHandle(accepted));

    TEST_ASSERT(SYS_SOCKET_IsReady(client) == 1);
    TEST_ASSERT(SYS_SOCKET_IsReady(accepted) == 1);
    TEST_ASSERT(SYS_SOCKET_IsWriteable(client, 1000) == 1);
    TEST_ASSERT(SYS_SOCKET_IsReadable(accepted, 0) == 0);
    TEST_ASSERT(SYS_SOCKET_SendAll(client,
                                   client_payload,
                                   (unsigned)(sizeof(client_payload) - 1u),
                                   1000) == (int)(sizeof(client_payload) - 1u));
    TEST_ASSERT(SYS_SOCKET_IsReadable(accepted, 1000) == 1);
    memset(buffer, 0, sizeof(buffer));
    TEST_ASSERT(SYS_SOCKET_Receive(accepted, buffer, sizeof(buffer)) ==
                (int)(sizeof(client_payload) - 1u));
    TEST_ASSERT(memcmp(buffer, client_payload, sizeof(client_payload) - 1u) == 0);

    TEST_ASSERT(SYS_SOCKET_Send(accepted,
                                server_payload,
                                (unsigned)(sizeof(server_payload) - 1u)) ==
                (int)(sizeof(server_payload) - 1u));
    TEST_ASSERT(SYS_SOCKET_IsReadable(client, 1000) == 1);
    memset(buffer, 0, sizeof(buffer));
    TEST_ASSERT(SYS_SOCKET_Receive(client, buffer, sizeof(buffer)) ==
                (int)(sizeof(server_payload) - 1u));
    TEST_ASSERT(memcmp(buffer, server_payload, sizeof(server_payload) - 1u) == 0);
    TEST_ASSERT(SYS_SOCKET_SendAll(client, client_payload, 0u, 0) == 0);

    SYS_SOCKET_Shutdown(client, SYS_SOCKET_SHUT_RD);
    SYS_SOCKET_Shutdown(accepted, SYS_SOCKET_SHUT_WR);
    SYS_SOCKET_Shutdown(client, SYS_SOCKET_SHUT_RDWR);
    SYS_SOCKET_Shutdown(client, 99);
    SYS_SOCKET_Close(accepted);
    SYS_SOCKET_Close(client);
    SYS_SOCKET_Close(listener);
    return 0;
}

int main(void) {
    TEST_RUN(_TestTelnetResetAndPlainData);
    TEST_RUN(_TestTelnetCommandFiltering);
    TEST_RUN(_TestTelnetSplitOptionAndResponseCapacity);
    TEST_RUN(_TestTelnetOptionResponseVariants);
    TEST_RUN(_TestTelnetSubnegotiationIacRecovery);
    TEST_RUN(_TestTelnetNegotiationInvalidSocket);
    TEST_RUN(_TestTelnetNegotiationSocketPair);
    TEST_RUN(_TestSocketInvalidHandleContracts);
    TEST_RUN(_TestSocketOpenAndClose);
    TEST_RUN(_TestSocketLoopbackDataFlow);
    return 0;
}

/*************************** End of file ****************************/
