#include "server_event.h"
#include "connection.h"
#include "log.h"
#include "protocol.h"
#include "server_packet.h"
#include "vector.h"

#include <stddef.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

bool handle_server_events(event_context_t *ctx) {
    struct pollfd server_pfd;
    struct pollfd client_pfd;

    vector_get(ctx->poll_fds, 0, &server_pfd);
    vector_get(ctx->poll_fds, 1, &client_pfd);

    if (!(server_pfd.revents & POLLIN)) return true;

    int socket_fd = accept(server_pfd.fd, NULL, NULL);
    if (socket_fd == -1) {
        log_error("failed to accept socket");
        return false;
    }

    log_info("accepted socket %d", socket_fd);

    struct pollfd socket_pfd = {.fd = socket_fd, .events = POLLIN};
    if (!vector_push(ctx->poll_fds, &socket_pfd)) {
        close(socket_fd);

        log_error("failed to add pollfd into pollfds vector");
        return false;
    }

    connection_t *conn = connection_create(ctx->server.next_connection_id, socket_pfd.fd);

    hashtable_put(ctx->connections_by_fd, &conn->fd, sizeof(int), &conn, sizeof(connection_t *));
    hashtable_put(ctx->connections_by_id, &conn->id, sizeof(uint32_t), &conn, sizeof(connection_t *));

    protocol_send_open(client_pfd.fd, conn->id);
    log_info("created connection %d", conn->id);

    return true;
}

bool handle_client_events(event_context_t *ctx) {
    struct pollfd client_pfd;
    vector_get(ctx->poll_fds, 1, &client_pfd);

    if (!(client_pfd.revents & POLLIN)) return true;

    packet_header_t header;
    if (!protocol_read_header(client_pfd.fd, &header)) return false;

    return handle_packet(ctx, &header);
}

bool handle_local_events(event_context_t *ctx) {
    struct pollfd client_pfd;
    vector_get(ctx->poll_fds, 1, &client_pfd);

    for (size_t i = 2; i < ctx->poll_fds->size; i++) {
        struct pollfd socket_pfd;
        vector_get(ctx->poll_fds, i, &socket_pfd);

        if (!(socket_pfd.revents & POLLIN)) continue;

        connection_t *conn = NULL;
        if (!hashtable_get(ctx->connections_by_fd, &socket_pfd.fd, sizeof(socket_pfd.fd), &conn)) {
            log_error("missing connection for socket %d", socket_pfd.fd);
            continue;
        }

        ssize_t n = read(socket_pfd.fd, conn->read_buffer, sizeof(conn->read_buffer));
        if (n == -1) {
            log_error("failed to read socket %d", socket_pfd.fd);
            continue;
        }

        if (n == 0) {
            connection_unregister(conn, ctx->poll_fds, ctx->connections_by_fd, ctx->connections_by_id);

            protocol_send_close(client_pfd.fd, conn->id);
            log_info("closed socket connection %d", conn->id);

            connection_destroy(conn);

            continue;
        }

        conn->read_buffer_length = (size_t)n;
        protocol_send_data(client_pfd.fd, conn->id, conn->read_buffer, conn->read_buffer_length);
        conn->read_buffer_length = 0;
    }

    return true;
}
