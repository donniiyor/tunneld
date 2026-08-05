#include "client_event.h"
#include "connection.h"
#include "hashtable.h"
#include "log.h"
#include "protocol.h"
#include "vector.h"

#include <arpa/inet.h>
#include <assert.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BACKLOG 8
#define SERVER_PORT 8080
#define SERVER_ADDRESS "127.0.0.1"
#define SERVICE_ADDRESS "127.0.0.1"

int main(int argc, char *argv[]) {
    assert(argc == 2);

    // Establish connection with server
    int server_fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_fd == -1) {
        log_errno("failed to create tunnel server socket");
        return EXIT_FAILURE;
    }

    struct sockaddr_in server_address = {.sin_family = AF_INET, .sin_port = htons(SERVER_PORT)};
    if (inet_pton(AF_INET, SERVER_ADDRESS, &server_address.sin_addr) <= 0) {
        log_error("invalid tunnel server address: address=%s", SERVER_ADDRESS);
        return EXIT_FAILURE;
    }

    if (connect(server_fd, (struct sockaddr *)&server_address, sizeof(server_address)) == -1) {
        log_errno("failed to connect to tunnel server: fd=%d target=%s:%d", server_fd, SERVER_ADDRESS, SERVER_PORT);
        return EXIT_FAILURE;
    }

    log_info("connected to tunnel server: server_fd=%d target=%s:%d local_service=%s:%d", server_fd, SERVER_ADDRESS,
             SERVER_PORT, SERVICE_ADDRESS, atoi(argv[1]));

    tunnel_endpoint_t endpoint;
    if (protocol_read_tunnel_endpoint(server_fd, &endpoint) == -1) {
        log_error("failed to receive allocated tunnel endpoint from server: server_fd=%d", server_fd);
        close(server_fd);
        return EXIT_FAILURE;
    }

    log_info("tunnel endpoint allocated: open http://%s:%u", endpoint.host, endpoint.port);

    hashtable_t *connections_by_fd = hashtable_create(sizeof(connection_t) * 10, BACKLOG);
    hashtable_t *connections_by_id = hashtable_create(sizeof(connection_t) * 10, BACKLOG);
    vector_t *poll_fds = vector_create(sizeof(struct pollfd), BACKLOG);

    struct pollfd server_pfd = {.fd = server_fd, .events = POLLIN};
    vector_push(poll_fds, &server_pfd);

    event_context_t ctx = {.server_fd = server_fd,
                           .poll_fds = poll_fds,
                           .connections_by_fd = connections_by_fd,
                           .connections_by_id = connections_by_id,
                           .localhost = SERVICE_ADDRESS,
                           .localport = atoi(argv[1])};

    while (1) {
        int ready = poll(poll_fds->data, poll_fds->size, -1);
        if (ready == -1) {
            log_errno("poll failed: watched_fds=%zu", poll_fds->size);
            return EXIT_FAILURE;
        }

        if (!handle_server_events(&ctx) || !handle_local_events(&ctx)) {
            log_error("event handling failed; shutting down client");
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}
