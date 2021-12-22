#ifndef __NETWORK_H__
#define __NETWORK_H__

#include <signal.h>
#include <stdbool.h>
#include <stdint.h>

#include "tcpsock.h"

#define NET_SUCCESS             0
#define NET_SOCKET_ERROR        1
#define NET_ADDRESS_ERROR       2
#define NET_SOCKOP_ERROR        3
#define NET_CONNECTION_CLOSED   4
#define NET_MEMORY_ERROR        5
#define NET_UNSPECIFIED_ERROR   16
#define NET_UNEXPECTED_NULL     17

#define NET_CB_SUCCESS          0
#define NET_CB_CLIENT_ERROR     0x04
#define NET_CB_DISCONNECT       0x08

/**
 * @brief Callback for when a client connects
 * 
 * @param client socket of the client that just connected
 */
typedef int (*callback_connected_t)(tcpsock_t* client);

/**
 * @brief Callback for when a client sends data
 * 
 * @param client socket of the client that sent the data
 * @param data pointer to a static buffer where the sent data can be read from
 * @param length length of the data buffer
 */
typedef int (*callback_data_t)(tcpsock_t* client, const void* data, unsigned int length);

/**
 * @brief Callback for when an error occured during handling of a client
 * 
 * @note client may disconnect afterwards
 * 
 * @param client socket of the client that triggered the error
 * @param err the net error which occured on the client
 */
typedef int (*callback_error_t)(tcpsock_t* client, int err);

/**
 * @brief Callback for when a client disconnects
 * 
 * @note the callback is called after disconnecting, so no data can be sent
 * 
 * @param client socket of the client that just disconnected
 */
typedef int (*callback_disconnected_t)(tcpsock_t* client);

typedef struct net_config {
    uint16_t port;          // port to open the server on
    bool verbose;           // enable verbose output
    sig_atomic_t running;   // keep running net_loop?

    callback_connected_t cb_connected;          
    callback_data_t cb_data;
    callback_error_t cb_error;
    callback_disconnected_t cb_disconnected;
} net_config_t;

const char* net_strerror(int net_error);
int net_loop(net_config_t* config);

#endif //__NETWORK_H__