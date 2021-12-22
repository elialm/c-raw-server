#include "network.h"

#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/select.h>

#include "log.h"
#include "tcpsock.h"
#include "vector.h"

#define SERVER_WELCOME_STRING "Successfully connected to server!\n"

#define READ_BUFFER_SIZE 1024

typedef enum client_action {
    CACT_NONE = 0,
    CACT_REMOVE
} client_action_t;

static int _reinterpret_error(int tcp_err)
{
    switch (tcp_err) {
        case TCP_NO_ERROR:          return NET_SUCCESS;
        case TCP_SOCKET_ERROR:      return NET_SOCKET_ERROR;
        case TCP_ADDRESS_ERROR:     return NET_ADDRESS_ERROR;
        case TCP_SOCKOP_ERROR:      return NET_SOCKOP_ERROR;
        case TCP_CONNECTION_CLOSED: return NET_CONNECTION_CLOSED;
        case TCP_MEMORY_ERROR:      return NET_MEMORY_ERROR;
        default:                    return NET_UNSPECIFIED_ERROR;
    }
}

static int _initialize_server(net_config_t* config, tcpsock_t* server_sock, vec_t* client_vec)
{
    int err = NET_SUCCESS;

    if ((err = tcp_passive_open(server_sock, config->port)) != TCP_NO_ERROR) {
        err = _reinterpret_error(err);
        goto server_sock_error;
    }

    vec_err_t vec_err;
    if ((vec_err = vec_create(client_vec, sizeof(tcpsock_t), DEFAULT_CAPACITY)) != VEC_ERR_SUCCESS) {
        err = NET_MEMORY_ERROR;
        goto client_vec_error;
    }

    goto success;

    client_vec_error:
    tcp_close(server_sock);

    server_sock_error:
    success:
    // no action taken

    return err;
}

static int _setup_fds(fd_set* fds, int server_fd, vec_t* client_vec)
{
    FD_ZERO(fds);
    FD_SET(server_fd, fds);
    int biggest_fd = server_fd;

    for (unsigned int i = 0; i < vec_size(client_vec); i++) {
        tcpsock_t* client_sock;
        vec_get_ref(client_vec, (void**)&client_sock, i);

        int client_sd = tcp_get_fd(client_sock);
        FD_SET(client_sd, fds);

        if (client_sd > biggest_fd) {
            biggest_fd = client_sd;
        }
    }

    return biggest_fd;
}

static int _accept_client(tcpsock_t* server_sock, vec_t* client_vec)
{
    tcpsock_t client_sock;
    int err = tcp_wait_for_connection(server_sock, &client_sock);

    if (err != TCP_NO_ERROR) {
        PRINTF_DEBUG("Server failed accepting client (%i), errno = %i", err, errno);
        // TODO: handle TCP_SOCKOP_ERROR and TCP_MEMORY_ERROR appropriately
    } else {
        int welcome_size = sizeof(SERVER_WELCOME_STRING);
        tcp_send(&client_sock, SERVER_WELCOME_STRING, &welcome_size);
        vec_push_back(client_vec, &client_sock);
    }

    return err;
}

static int _handle_client(tcpsock_t* client_sock, int client_fd, net_config_t* config)
{
    uint8_t buff[READ_BUFFER_SIZE];
    unsigned int buff_size = READ_BUFFER_SIZE;
    
    int err = tcp_receive(client_sock, buff, &buff_size);

    switch (err) {
        case TCP_CONNECTION_CLOSED:
            PRINTF_DEBUG("Client (fd = %i) disconnected", client_fd);
            tcp_close(client_sock);
            return CACT_REMOVE;

        case TCP_SOCKOP_ERROR:
            PRINTF_DEBUG("Client (fd = %i) failed socket operation while reading, errno = %i", client_fd, errno);
            tcp_close(client_sock);
            return CACT_REMOVE;

        case TCP_NO_ERROR:
            PRINTF_DEBUG("Client (fd = %i) sent %i bytes", client_sock->fd, buff_size);
            if (config->cb_data) {
                config->cb_data(client_sock, buff, buff_size);
            }
            return CACT_NONE;

        default:
            PRINTF_DEBUG("Unhandled tcp_receive error, code = %i", err);
            tcp_close(client_sock);
            return CACT_REMOVE;
    }
}

static int _listen_loop(tcpsock_t* server_sock, vec_t* client_vec, net_config_t* config)
{
    int sock_err;
    int net_err = NET_SUCCESS;
    int server_fd = tcp_get_fd(server_sock);

    while (config->running) {
        fd_set fds;
        int biggest_fd = _setup_fds(&fds, server_fd, client_vec);

        int activity = select(biggest_fd + 1, &fds, NULL, NULL, NULL);

        if (activity > 0 && FD_ISSET(server_fd, &fds)) {
            sock_err = _accept_client(server_sock, client_vec);
            if (sock_err != TCP_NO_ERROR) {
                PRINTF_DEBUG("Failure when accepting client, error code %i", sock_err);
                // TODO: handle accept error, maybe ignore?
            } else {
                PRINTF_DEBUG("New client connected!");
            }

            activity--;
        }

        unsigned int i = 0;
        while (activity > 0 && i < vec_size(client_vec)) {
            tcpsock_t* client_sock;
            vec_get_ref(client_vec, (void**)&client_sock, i);

            int client_fd = tcp_get_fd(client_sock);
            int client_action = CACT_NONE;
            if (FD_ISSET(client_fd, &fds)) {
                client_action = _handle_client(client_sock, client_fd, config);
                activity--;
            }

            if (client_action == CACT_REMOVE) {
                vec_remove(client_vec, i);
            } else {
                i++;
            }
        }
    }

    // cleanup
    for (unsigned int i = 0; i < vec_size(client_vec); i++) {
        tcpsock_t* client_sock;
        vec_get_ref(client_vec, (void**)&client_sock, i);

        tcp_close(client_sock);
    }

    vec_destroy(client_vec);
    tcp_close(server_sock);

    return net_err;
}

const char* net_strerror(int net_error)
{
    switch (net_error) {
        case NET_SUCCESS:           return "NET_SUCCESS";
        case NET_SOCKET_ERROR:      return "NET_SOCKET_ERROR";
        case NET_ADDRESS_ERROR:     return "NET_ADDRESS_ERROR";
        case NET_SOCKOP_ERROR:      return "NET_SOCKOP_ERROR";
        case NET_CONNECTION_CLOSED: return "NET_CONNECTION_CLOSED";
        case NET_MEMORY_ERROR:      return "NET_MEMORY_ERROR";
        case NET_UNSPECIFIED_ERROR: return "NET_UNSPECIFIED_ERROR";
        case NET_UNEXPECTED_NULL:   return "NET_UNEXPECTED_NULL";
        default:                    return "<error>";
    }
}

int net_loop(net_config_t* config)
{
    if (config == NULL) {
        return NET_UNEXPECTED_NULL;
    }

    tcpsock_t server_sock;
    vec_t client_vec;
    int err = _initialize_server(config, &server_sock, &client_vec);
    if (err != NET_SUCCESS) {
        return err;
    }

    return _listen_loop(&server_sock, &client_vec, config);
}