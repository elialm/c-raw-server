#define _GNU_SOURCE

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "log.h"
#include "tcpsock.h"

#define CHAR_IP_ADDR_LENGTH  16              // 4 numbers of 3 digits, 3 dots and \0
#define PROTOCOLFAMILY       AF_INET         // internet protocol suite
#define TYPE                 SOCK_STREAM     // streaming protool type
#define PROTOCOL             IPPROTO_TCP     // TCP protocol

#define NO_ACTION ((void)0)
#define HANDLE_ERROR(condition, additional_action, format, ...)                     \
do {                                                                        \
    if ((condition)) {                                                      \
        PRINTF_DEBUG(#condition " failed : " format, __VA_ARGS__);           \
        additional_action;                                                  \
    }                                                                       \
} while (0)

#define HANDLE_ERROR_GOTO(condition, additional_action, label, format, ...) \
    HANDLE_ERROR(condition, additional_action; goto label, format, __VA_ARGS__)

#define CHECK_FOR_ERROR(condition, format, ...) \
    HANDLE_ERROR(condition, NO_ACTION, format, __VA_ARGS__)

int tcp_passive_open(tcpsock_t* sock, const uint16_t port)
{
    if (sock == NULL) {
        return TCP_SOCKET_ERROR;
    }

    if (port < MIN_PORT || port > MAX_PORT) {
        return TCP_ADDRESS_ERROR;
    }

    int result;
    int err = TCP_NO_ERROR;
    sock->connected = false;

    sock->fd = socket(PROTOCOLFAMILY, TYPE, PROTOCOL);
    HANDLE_ERROR_GOTO(sock->fd < 0, err = TCP_SOCKOP_ERROR, socket_creation_error,
                "call to socket() failed with errno = %i", errno);

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = PROTOCOLFAMILY;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);
    
    result = bind(sock->fd, (struct sockaddr*)&addr, sizeof(addr));
    HANDLE_ERROR_GOTO(result == -1, err = TCP_SOCKOP_ERROR, socket_binding_error,
                "call to bind() failed with errno = %i", errno);

    result = listen(sock->fd, MAX_PENDING);
    HANDLE_ERROR_GOTO(result == -1, err = TCP_SOCKOP_ERROR, socket_listening_error,
                "call to listen() failed with errno = %i", errno);

    sock->connected = true;
    sock->ip_addr = NULL;
    sock->port = port;
    goto success;

    socket_listening_error:
    socket_binding_error:
    socket_creation_error:
    ip_memory_alloc_error:
    success:
    // do nothing

    return err;
}

/**
    int tcp_passive_open(tcpsock_t **sock, int port) {
        int result;
        struct sockaddr_in addr;
        TCP_ERR_HANDLER(((port < MIN_PORT) || (port > MAX_PORT)), return TCP_ADDRESS_ERROR);
        tcpsock_t *s = tcp_sock_create();
        TCP_ERR_HANDLER(s == NULL, return TCP_MEMORY_ERROR);
        s->sd = socket(PROTOCOLFAMILY, TYPE, PROTOCOL);
        TCP_DEBUG_PRINTF(s->sd < 0, "Socket() failed with errno = %d [%s]", errno, strerror(errno));
        TCP_ERR_HANDLER(s->sd < 0, free(s);return TCP_SOCKOP_ERROR);
        // Construct the server address structure
        memset(&addr, 0, sizeof(struct sockaddr_in));
        addr.sin_family = PROTOCOLFAMILY;
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        addr.sin_port = htons(port);
        result = bind(s->sd, (struct sockaddr *) &addr, sizeof(addr));
        TCP_DEBUG_PRINTF(result == -1, "Bind() failed with errno = %d [%s]", errno, strerror(errno));
        TCP_ERR_HANDLER(result != 0, free(s);return TCP_SOCKOP_ERROR);
        result = listen(s->sd, MAX_PENDING);
        TCP_DEBUG_PRINTF(result == -1, "Listen() failed with errno = %d [%s]", errno, strerror(errno));
        TCP_ERR_HANDLER(result != 0, free(s);return TCP_SOCKOP_ERROR);
        s->ip_addr = NULL; // address set to INADDR_ANY - not a specific IP address
        s->port = port;
        s->cookie = MAGIC_COOKIE;
        *sock = s;
        return TCP_NO_ERROR;
    }
 */

int tcp_active_open(tcpsock_t* sock, const uint16_t remote_port, const char* remote_ip)
{
    // TODO: implement
    return TCP_SOCKET_ERROR;
}

int tcp_close(tcpsock_t* sock)
{
    if (sock == NULL) {
        return TCP_SOCKET_ERROR;
    }

    if (sock->connected) {
        int result = shutdown(sock->fd, SHUT_RDWR);
        CHECK_FOR_ERROR(result == -1,
            "call to shutdown() failed with errno = %i [%s]", errno, strerror(errno));

        if (result != -1) {
            result = close(sock->fd);
            CHECK_FOR_ERROR(result == -1,
                "call to close() failed with errno = %i [%s]", errno, strerror(errno));
        }
    }

    if (sock->ip_addr != NULL) {
        free(sock->ip_addr);
    }

    sock->connected = false;
    sock->fd = -1;
    sock->ip_addr = NULL;
    sock->port = 0;

    return TCP_NO_ERROR;
}

int tcp_wait_for_connection(tcpsock_t* sock, tcpsock_t* new_sock)
{
    if (sock == NULL || new_sock == NULL) {
        return TCP_SOCKET_ERROR;
    }

    int err = TCP_NO_ERROR;
    struct sockaddr_in addr;
    unsigned int length = sizeof(struct sockaddr_in);


    new_sock->fd = accept(sock->fd, (struct sockaddr*)&addr, &length);
    HANDLE_ERROR_GOTO(new_sock->fd == -1, err = TCP_SOCKOP_ERROR, socket_accept_error,
        "call to accept() failed with errno = %i [%s]", errno, strerror(errno));
    
    const char* ip_addr = inet_ntoa(addr.sin_addr);
    new_sock->ip_addr = malloc(CHAR_IP_ADDR_LENGTH);
    HANDLE_ERROR_GOTO(new_sock->ip_addr == NULL, err = TCP_MEMORY_ERROR, ip_buff_malloc_error,
        "Failed to allocation memory for ip char buffer");

    strncpy(new_sock->ip_addr, ip_addr, CHAR_IP_ADDR_LENGTH);
    new_sock->port = ntohs(addr.sin_port);
    new_sock->connected = true;
    goto success;

    ip_buff_malloc_error:
    socket_accept_error:
    success:
    // do nothing

    return err;
}

int tcp_send(tcpsock_t* sock, const void* buffer, unsigned int* buff_size)
{
    if (sock == NULL || buff_size == NULL) {
        return TCP_SOCKET_ERROR;
    }

    if (!sock->connected) {
        return TCP_SOCKET_ERROR;
    }

    int err = TCP_NO_ERROR;
    if (buffer != NULL && *buff_size != 0) {
        *buff_size = sendto(sock->fd, buffer, *buff_size, MSG_NOSIGNAL, NULL, 0);
        
        HANDLE_ERROR_GOTO(*buff_size == 0, err = TCP_CONNECTION_CLOSED, zero_bytes_sent,
            "call to sendto() returned 0 sent bytes : connection with peer is closed");
        HANDLE_ERROR_GOTO(*buff_size < 0 && ((errno == EPIPE) || (errno == ENOTCONN)),
            err = TCP_CONNECTION_CLOSED, sendto_pipe_notconn_error,
            "call to sendto() returned errno = %i [%s] : connection with peer is closed",
            errno, strerror(errno));
        HANDLE_ERROR_GOTO(*buff_size < 0, err = TCP_SOCKOP_ERROR, sendto_other_error,
            "call to sendto() returned errno = %i [%s]", errno, strerror(errno));
    } else {
        *buff_size = 0;
    }

    goto success;

    zero_bytes_sent:
    sendto_pipe_notconn_error:
    sendto_other_error:
    success:
    // do nothing

    return err;
}

int tcp_receive(tcpsock_t* sock, void* buffer, unsigned int* buff_size)
{
    if (sock == NULL || buffer == NULL || buff_size == NULL) {
        return TCP_SOCKET_ERROR;
    }

    if (!sock->connected) {
        return TCP_SOCKET_ERROR;
    }

    int err = TCP_NO_ERROR;
    if (buffer != NULL && *buff_size != 0) {
        *buff_size = recv(sock->fd, buffer, *buff_size, 0);
        
        HANDLE_ERROR_GOTO(*buff_size == 0, err = TCP_CONNECTION_CLOSED, zero_bytes_sent,
            "call to recv() returned 0 received bytes : connection with peer is closed");
        HANDLE_ERROR_GOTO(*buff_size < 0 && (errno == ENOTCONN),
            err = TCP_CONNECTION_CLOSED, recv_notconn_error,
            "call to recv() returned errno = %i [%s] : connection with peer is closed",
            errno, strerror(errno));
        HANDLE_ERROR_GOTO(*buff_size < 0, err = TCP_SOCKOP_ERROR, recv_other_error,
            "call to recv() returned errno = %i [%s]", errno, strerror(errno));
    } else {
        *buff_size = 0;
    }

    goto success;

    zero_bytes_sent:
    recv_notconn_error:
    recv_other_error:
    success:
    // do nothing

    return err;
}

char* tcp_get_ip_addr(tcpsock_t* sock)
{
    if (sock == NULL) {
        return NULL;
    }

    return sock->ip_addr;
}

uint16_t tcp_get_port(tcpsock_t* sock)
{
    if (sock == NULL) {
        return 0;
    }

    return sock->port;
}

int tcp_get_fd(tcpsock_t* sock)
{
    if (sock == NULL) {
        return -1;
    }

    return sock->fd;
}