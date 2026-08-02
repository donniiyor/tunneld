#include "client_event.h"
#include "client_packet.h"
#include "connection.h"
#include "protocol.h"

#include <arpa/inet.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

bool handle_server_events(event_context_t *ctx) {
    struct pollfd server_pfd;
    vector_get(ctx->poll_fds, 0, &server_pfd);

    if (!(server_pfd.revents & POLLIN)) return true;

    packet_header_t header;
    if (!protocol_read_header(server_pfd.fd, &header)) return false;

    return handle_packet(ctx, &header);
}

bool handle_local_events(event_context_t *ctx) {
    for (size_t i = 1; i < ctx->poll_fds->size; i++) {
        struct pollfd local_pfd;
        vector_get(ctx->poll_fds, i, &local_pfd);

        if (local_pfd.revents & POLLIN) {
            connection_t *conn = NULL;
            if (!hashtable_get(ctx->connections_by_fd, &local_pfd.fd, sizeof(int), &conn)) continue;

            ssize_t n = read(local_pfd.fd, conn->read_buffer, sizeof(conn->read_buffer));
            if (n == -1) {
                perror("read local connection");
                continue;
            }

            if (n == 0) {
                connection_unregister(conn, ctx->poll_fds, ctx->connections_by_fd, ctx->connections_by_id);

                protocol_send_close(ctx->server_fd, conn->id);

                printf("closed local connection: %d\n", conn->id);

                connection_destroy(conn);

                continue;
            }

            conn->read_buffer_length = n;
            protocol_send_data(ctx->server_fd, conn->id, conn->read_buffer, conn->read_buffer_length);
            conn->read_buffer_length = 0;
        }
    }

    return true;
}
