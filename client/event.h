#ifndef EVENT_H
#define EVENT_H

#include "hashtable.h"
#include "vector.h"

#include <arpa/inet.h>
#include <poll.h>
#include <stdint.h>

typedef struct {
    int server_fd;

    vector_t *poll_fds;

    hashtable_t *connections_by_id;
    hashtable_t *connections_by_fd;

    char localhost[INET_ADDRSTRLEN];
    uint16_t localport;
} event_context_t;

bool handle_server_events(event_context_t *ctx);
bool handle_local_events(event_context_t *ctx);

#endif
