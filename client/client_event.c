#include "client_event.h"
#include "client_packet.h"
#include "connection.h"
#include "log.h"
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
    if (protocol_read_header(server_pfd.fd, &header) == -1) {
        log_error("failed to read packet header from tunnel server fd %d", server_pfd.fd);
        return false;
    }

    log_info("received packet from tunnel server: fd=%d type=%d conn=%u bytes=%u", server_pfd.fd, header.type,
             header.connection_id, header.length);

    return handle_packet(ctx, &header);
}

bool handle_local_events(event_context_t *ctx) {
    for (size_t i = 1; i < ctx->poll_fds->size; i++) {
        struct pollfd local_pfd;
        vector_get(ctx->poll_fds, i, &local_pfd);

        if (local_pfd.revents & POLLIN) {
            connection_t *conn = NULL;
            if (!hashtable_get(ctx->connections_by_fd, &local_pfd.fd, sizeof(int), &conn)) {
                log_error("missing connection for local service socket: fd=%d", local_pfd.fd);
                continue;
            }

            ssize_t n = read(local_pfd.fd, conn->read_buffer, sizeof(conn->read_buffer));
            if (n == -1) {
                log_errno("failed to read local service socket: conn=%u fd=%d", conn->id, local_pfd.fd);
                continue;
            }

            if (n == 0) {
                log_info("local service closed socket: conn=%u local_fd=%d", conn->id, conn->fd);
                connection_unregister(conn, ctx->poll_fds, ctx->connections_by_fd, ctx->connections_by_id);

                if (protocol_send_close(ctx->server_fd, conn->id) == -1) {
                    log_error("failed to notify tunnel server about local EOF: conn=%u server_fd=%d", conn->id,
                              ctx->server_fd);
                }

                log_info("closed local connection after EOF: conn=%u local_fd=%d server_fd=%d", conn->id, local_pfd.fd,
                         ctx->server_fd);

                connection_destroy(conn);

                continue;
            }

            conn->read_buffer_length = n;
            log_info("forwarding local service data to tunnel server: conn=%u local_fd=%d server_fd=%d bytes=%zu",
                     conn->id, conn->fd, ctx->server_fd, conn->read_buffer_length);
            if (protocol_send_data(ctx->server_fd, conn->id, conn->read_buffer, conn->read_buffer_length) == -1) {
                log_error("failed to forward local service data to tunnel server: conn=%u local_fd=%d server_fd=%d "
                          "bytes=%zu",
                          conn->id, conn->fd, ctx->server_fd, conn->read_buffer_length);
            }
            conn->read_buffer_length = 0;
        }
    }

    return true;
}
