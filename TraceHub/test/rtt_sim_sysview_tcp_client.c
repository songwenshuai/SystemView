/*********************************************************************
*                                                                    *
*                Copyright (C) 2023 xrTest Inc.                      *
*                      All rights reserved                           *
*                                                                    *
**********************************************************************
----------------------------------------------------------------------
File    : rtt_sim_sysview_tcp_client.c
Purpose : TCP client smoke validator for the tracehub SystemView service
Author  : songwenshuai <songwenshuai@gmail.com>
---------------------------END-OF-HEADER------------------------------
*/

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "SystemView.h"

/*********************************************************************
*
*       Defines
*
**********************************************************************
*/

#define SYSVIEW_TCP_DEFAULT_HOST        "127.0.0.1"
#define SYSVIEW_TCP_DEFAULT_PORT        19111u
#define SYSVIEW_TCP_DEFAULT_MIN_BYTES   1u
#define SYSVIEW_TCP_DEFAULT_TIMEOUT_MS  10000u
#define SYSVIEW_TCP_CONNECT_RETRY_MS    100u

/*********************************************************************
*
*       Static functions
*
**********************************************************************
*/

/*********************************************************************
*
*       _NowMs()
*
*  Function description
*    Return monotonic elapsed time in milliseconds.
*/
static uint64_t _NowMs(void) {
    struct timespec ts;

    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        return 0u;
    }
    return ((uint64_t)ts.tv_sec * 1000u) + ((uint64_t)ts.tv_nsec / 1000000u);
}

/*********************************************************************
*
*       _SleepMs()
*
*  Function description
*    Sleep for the requested number of milliseconds.
*/
static void _SleepMs(unsigned timeout_ms) {
    struct timespec req;

    req.tv_sec = (time_t)(timeout_ms / 1000u);
    req.tv_nsec = (long)(timeout_ms % 1000u) * 1000000L;
    (void)nanosleep(&req, NULL);
}

/*********************************************************************
*
*       _GetRemainingTimeoutMs()
*
*  Function description
*    Return remaining milliseconds before the deadline expires.
*/
static unsigned _GetRemainingTimeoutMs(uint64_t deadline) {
    uint64_t now;
    uint64_t remaining;

    now = _NowMs();
    if (now >= deadline) {
        return 0u;
    }

    remaining = deadline - now;
    if (remaining > UINT32_MAX) {
        return UINT32_MAX;
    }
    return (unsigned)remaining;
}

/*********************************************************************
*
*       _ParseUnsigned()
*
*  Function description
*    Parse an unsigned decimal argument.
*/
static int _ParseUnsigned(const char *str, unsigned *value) {
    char          *end;
    unsigned long parsed;

    if ((str == NULL) || (value == NULL) || (str[0] == '\0')) {
        return -1;
    }

    errno = 0;
    parsed = strtoul(str, &end, 10);
    if ((errno != 0) || (end == str) || (*end != '\0') || (parsed > UINT32_MAX)) {
        return -1;
    }

    *value = (unsigned)parsed;
    return 0;
}

/*********************************************************************
*
*       _ConnectOnce()
*
*  Function description
*    Attempt one blocking IPv4 TCP connection.
*/
static int _ConnectOnce(const char *host, unsigned port) {
    int                fd;
    struct sockaddr_in addr;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        return -1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)port);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    if ((strcmp(host, "127.0.0.1") != 0) &&
        (strcmp(host, "localhost") != 0)) {
        fprintf(stderr, "Only localhost SystemView smoke connections are supported: %s\n", host);
        close(fd);
        return -1;
    }

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        close(fd);
        return -1;
    }

    return fd;
}

/*********************************************************************
*
*       _ConnectWithRetry()
*
*  Function description
*    Connect to the SystemView TCP service before the timeout expires.
*/
static int _ConnectWithRetry(const char *host, unsigned port, unsigned timeout_ms) {
    uint64_t deadline;
    unsigned remaining_ms;
    int      fd;

    deadline = _NowMs() + timeout_ms;
    do {
        fd = _ConnectOnce(host, port);
        if (fd >= 0) {
            return fd;
        }
        remaining_ms = _GetRemainingTimeoutMs(deadline);
        if (remaining_ms == 0u) {
            break;
        }
        if (remaining_ms > SYSVIEW_TCP_CONNECT_RETRY_MS) {
            remaining_ms = SYSVIEW_TCP_CONNECT_RETRY_MS;
        }
        _SleepMs(remaining_ms);
    } while (true);

    return -1;
}

/*********************************************************************
*
*       _WaitForSocket()
*
*  Function description
*    Wait until a socket is readable or writable.
*/
static int _WaitForSocket(int fd, bool writable, unsigned timeout_ms) {
    fd_set         read_set;
    fd_set         write_set;
    fd_set        *read_ptr;
    fd_set        *write_ptr;
    struct timeval timeout;

    FD_ZERO(&read_set);
    FD_ZERO(&write_set);
    read_ptr = NULL;
    write_ptr = NULL;
    if (writable) {
        FD_SET(fd, &write_set);
        write_ptr = &write_set;
    } else {
        FD_SET(fd, &read_set);
        read_ptr = &read_set;
    }

    timeout.tv_sec = (time_t)(timeout_ms / 1000u);
    timeout.tv_usec = (suseconds_t)(timeout_ms % 1000u) * 1000;

    return select(fd + 1, read_ptr, write_ptr, NULL, &timeout);
}

/*********************************************************************
*
*       _SendAll()
*
*  Function description
*    Send every byte in the provided buffer before the timeout expires.
*/
static int _SendAll(int fd, const unsigned char *data, unsigned num_bytes, unsigned timeout_ms) {
    uint64_t deadline;
    unsigned offset;
    unsigned remaining_ms;
    ssize_t  sent;

    if ((data == NULL) || (num_bytes == 0u)) {
        return -1;
    }

    deadline = _NowMs() + timeout_ms;
    offset = 0u;
    while (offset < num_bytes) {
        remaining_ms = _GetRemainingTimeoutMs(deadline);
        if (remaining_ms == 0u) {
            return -1;
        }
        if (_WaitForSocket(fd, true, remaining_ms) <= 0) {
            return -1;
        }
        sent = send(fd, data + offset, (size_t)(num_bytes - offset), 0);
        if (sent <= 0) {
            return -1;
        }
        offset += (unsigned)sent;
    }

    return 0;
}

/*********************************************************************
*
*       _ReceiveExact()
*
*  Function description
*    Receive the exact number of requested bytes before the timeout expires.
*/
static int _ReceiveExact(int fd, unsigned char *data, unsigned num_bytes, unsigned timeout_ms) {
    uint64_t deadline;
    unsigned offset;
    unsigned remaining_ms;
    ssize_t  received;

    if ((data == NULL) || (num_bytes == 0u)) {
        return -1;
    }

    deadline = _NowMs() + timeout_ms;
    offset = 0u;
    while (offset < num_bytes) {
        remaining_ms = _GetRemainingTimeoutMs(deadline);
        if (remaining_ms == 0u) {
            return -1;
        }
        if (_WaitForSocket(fd, false, remaining_ms) <= 0) {
            return -1;
        }
        received = recv(fd, data + offset, (size_t)(num_bytes - offset), 0);
        if (received <= 0) {
            return -1;
        }
        offset += (unsigned)received;
    }

    return 0;
}

/*********************************************************************
*
*       _ReceiveTraceBytes()
*
*  Function description
*    Receive at least min_bytes trace bytes before the timeout expires.
*/
static int _ReceiveTraceBytes(int fd, unsigned min_bytes, unsigned timeout_ms, unsigned *received_bytes) {
    unsigned char buffer[4096];
    uint64_t      deadline;
    unsigned      remaining_ms;
    unsigned      total;
    ssize_t       received;

    if ((min_bytes == 0u) || (received_bytes == NULL)) {
        return -1;
    }

    deadline = _NowMs() + timeout_ms;
    total = 0u;
    while (total < min_bytes) {
        remaining_ms = _GetRemainingTimeoutMs(deadline);
        if (remaining_ms == 0u) {
            *received_bytes = total;
            return -1;
        }
        if (_WaitForSocket(fd, false, remaining_ms) <= 0) {
            *received_bytes = total;
            return -1;
        }
        received = recv(fd, buffer, sizeof(buffer), 0);
        if (received <= 0) {
            *received_bytes = total;
            return -1;
        }
        total += (unsigned)received;
    }

    *received_bytes = total;
    return 0;
}

/*********************************************************************
*
*       _Usage()
*
*  Function description
*    Print command usage.
*/
static void _Usage(const char *program) {
    fprintf(stderr,
            "Usage: %s [host] [port] [min_trace_bytes] [timeout_ms]\n",
            program);
}

/*********************************************************************
*
*       Public code
*
**********************************************************************
*/

/*********************************************************************
*
*       main()
*/
int main(int argc, char **argv) {
    const char    *host;
    unsigned       port;
    unsigned       min_bytes;
    unsigned       timeout_ms;
    unsigned       received_bytes;
    unsigned char  client_hello[SYSVIEW_HELLO_SIZE];
    unsigned char  hello_response[SYSVIEW_HELLO_SIZE];
    int            fd;

    host = SYSVIEW_TCP_DEFAULT_HOST;
    port = SYSVIEW_TCP_DEFAULT_PORT;
    min_bytes = SYSVIEW_TCP_DEFAULT_MIN_BYTES;
    timeout_ms = SYSVIEW_TCP_DEFAULT_TIMEOUT_MS;

    if (argc > 5) {
        _Usage(argv[0]);
        return 1;
    }
    if (argc >= 2) {
        host = argv[1];
    }
    if ((argc >= 3) && (_ParseUnsigned(argv[2], &port) != 0)) {
        fprintf(stderr, "Invalid port: %s\n", argv[2]);
        return 1;
    }
    if ((port == 0u) || (port > 65535u)) {
        fprintf(stderr, "Port must be in range 1..65535: %u\n", port);
        return 1;
    }
    if ((argc >= 4) && ((_ParseUnsigned(argv[3], &min_bytes) != 0) || (min_bytes == 0u))) {
        fprintf(stderr, "Invalid min_trace_bytes: %s\n", argv[3]);
        return 1;
    }
    if ((argc >= 5) && ((_ParseUnsigned(argv[4], &timeout_ms) != 0) || (timeout_ms == 0u))) {
        fprintf(stderr, "Invalid timeout_ms: %s\n", argv[4]);
        return 1;
    }

    signal(SIGPIPE, SIG_IGN);

    if (!SystemView_BuildHelloMessage(client_hello, sizeof(client_hello))) {
        fprintf(stderr, "Failed to build SystemView Hello message\n");
        return 1;
    }

    fd = _ConnectWithRetry(host, port, timeout_ms);
    if (fd < 0) {
        fprintf(stderr, "Failed to connect to SystemView TCP service at %s:%u\n",
                host,
                port);
        return 1;
    }

    if (_SendAll(fd, client_hello, SYSVIEW_HELLO_SIZE, timeout_ms) != 0) {
        fprintf(stderr, "Failed to send SystemView Hello message\n");
        close(fd);
        return 1;
    }

    if (_ReceiveExact(fd, hello_response, SYSVIEW_HELLO_SIZE, timeout_ms) != 0) {
        fprintf(stderr, "Failed to receive SystemView Hello response\n");
        close(fd);
        return 1;
    }
    if (memcmp(hello_response, client_hello, SYSVIEW_HELLO_SIZE) != 0) {
        fprintf(stderr, "Unexpected SystemView Hello response\n");
        close(fd);
        return 1;
    }

    if (_ReceiveTraceBytes(fd, min_bytes, timeout_ms, &received_bytes) != 0) {
        fprintf(stderr,
                "SystemView TCP trace data was not received: got %u bytes, expected at least %u\n",
                received_bytes,
                min_bytes);
        close(fd);
        return 1;
    }

    printf("SystemView TCP smoke received %u trace bytes\n", received_bytes);
    close(fd);
    return 0;
}

/*************************** End of file ****************************/
