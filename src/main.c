#define _POSIX_SOURCE

#include <stdio.h>
#include <argp.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include "network.h"
#include "log.h"
#include "res.h"

#define SERVER_RESPONSE_STRING "Message received\n"

void* malloc_safe(unsigned int size)
{
    void* mem = malloc(size);
    if (mem == NULL) {
        PRINTF_DEBUG("unable to allocate %u bytes of memory in malloc_safe() : aborting...", size);
        abort();

        // suppresses compiler warnings
        return NULL;
    }

    return mem;
}

static char error_msg[64] = "";
static char doc[] = RES_DOC;
static char args_doc[] = RES_ARGS_DOC;

static struct argp_option options[] = {
    {"verbose", 'v', 0, 0, RES_ARGP_OPTIONS_VERBOSE},
    {0}
};

static error_t _parse_port(const char* port_str, uint16_t* port)
{
    uint16_t p = atoi(port_str);
    if (p < 1024) {
        sprintf(error_msg, RES_ARGP_PORT_ERROR_FORMAT, port_str);
        return EINVAL;
    }

    *port = p;
    return 0;
}

static error_t _parse_opt (int key, char *arg, struct argp_state *state)
{
    net_config_t *arguments = state->input;

    switch (key) {
        case 'v':
            arguments->verbose = true;
            break;

        case ARGP_KEY_ARG:
            if (state->arg_num >= 1) {
                argp_usage(state);
            }

            error_t err = _parse_port(arg, &arguments->port);
            if (err != 0) {
                return err;
            }
            break;

        case ARGP_KEY_END:
            if (state->arg_num < 1) {
                argp_usage(state);
            }
            break;

        default:
            return ARGP_ERR_UNKNOWN; 
    }

    return 0;
}

static struct argp argp = { options, _parse_opt, args_doc, doc };

static int _callback_connected(tcpsock_t* client)
{
    // TODO: implement   
}

static int _callback_data(tcpsock_t* client, const void* data, unsigned int length)
{
    char* msg = malloc_safe(length + 1);
    strncpy(msg, data, length);

    int flags = NET_CB_SUCCESS;

    printf(" > %s", msg);

    unsigned int buff_size = sizeof(SERVER_RESPONSE_STRING);
    int err = tcp_send(client, SERVER_RESPONSE_STRING, &buff_size);
    if (err != TCP_NO_ERROR) {
        printf("Failed to send response to client\n");
        flags |= NET_CB_DISCONNECT | NET_CB_CLIENT_ERROR;
    } else {
        printf(" < %s", SERVER_RESPONSE_STRING);
    }

    return flags;
}

static int _callback_error(tcpsock_t* client, int err)
{
    // TODO: implement
}

static int _callback_disconnected(tcpsock_t* client)
{
    // TODO: implement
}

static net_config_t arguments = {
    .verbose = false,
    .running = true,
    .cb_connected = _callback_connected,
    .cb_data = _callback_data,
    .cb_error = _callback_error,
    .cb_disconnected = _callback_disconnected
};

static void _signal_handler(int signum)
{
    switch (signum) {
        case SIGINT:
            PRINTF_DEBUG("Interrupt received, signal to close program...");
            arguments.running = false;
            break;

        default:
            PRINTF_DEBUG("Non-handled signal %i", signum);
            break;
    }
}

int main(int argc, char **argv)
{
    struct sigaction act;
    act.sa_handler = _signal_handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;

    sigaction(SIGINT, &act, NULL);
    
    error_t err = argp_parse(&argp, argc, argv, 0, 0, &arguments);

    if (err != 0) {
        printf("%s: %s\n", argv[0], error_msg[0] != '\0' ? error_msg : RES_ARGP_UNSPECIFIED_ERROR);
        return err;
    }

    PRINTF_DEBUG("options:\n\t> verbose: %s\n\t> port: %u", 
            arguments.verbose ? "yes" : "no",
            arguments.port);

    return net_loop(&arguments);
}