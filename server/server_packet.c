#include "server_packet.h"
#include "connection.h"
#include "log.h"
#include "protocol.h"
#include "server_event.h"
#include "vector.h"

#include <errno.h>
#include <stdio.h>
#include <unistd.h>

static bool handle_data(event_context_t *ctx, const packet_header_t *header) {
    connection_t *conn = NULL;
    if (!hashtable_get(ctx->connections_by_id, &header->connection_id, sizeof(uint32_t), &conn)) {
        log_error("received DATA for unknown connection: conn=%u bytes=%u", header->connection_id, header->length);
        return false;
    }

    struct pollfd client_pfd;
    vector_get(ctx->poll_fds, 1, &client_pfd);

    if (header->length > sizeof(conn->write_buffer)) {
        log_error("packet payload too large from tunnel client: conn=%u bytes=%u limit=%zu", header->connection_id,
                  header->length, sizeof(conn->write_buffer));
        return false;
    }

    if (protocol_read_payload(client_pfd.fd, conn->write_buffer, header->length) == -1) {
        log_error("failed to read DATA payload from tunnel client: conn=%u client_fd=%d bytes=%u", conn->id,
                  client_pfd.fd, header->length);
        return false;
    }

    conn->write_buffer_length = header->length;
    log_info("forwarding tunnel data to public socket: conn=%u public_fd=%d client_fd=%d bytes=%zu", conn->id,
             conn->fd, client_pfd.fd, conn->write_buffer_length);

    ssize_t n = write(conn->fd, conn->write_buffer, conn->write_buffer_length);
    if (n == -1) {
        log_errno("failed to write tunnel data to public socket: conn=%u public_fd=%d bytes=%zu", conn->id, conn->fd,
                  conn->write_buffer_length);
        return false;
    }

    if ((size_t)n != conn->write_buffer_length) {
        log_error("short write to public socket: conn=%u public_fd=%d wrote=%zd expected=%zu", conn->id, conn->fd, n,
                  conn->write_buffer_length);
    }

    conn->write_buffer_length = 0;

    return true;
}

static bool handle_close(event_context_t *ctx, const packet_header_t *header) {
    connection_t *conn = NULL;
    if (!hashtable_get(ctx->connections_by_id, &header->connection_id, sizeof(uint32_t), &conn)) {
        log_error("received CLOSE for unknown connection: conn=%u", header->connection_id);
        return false;
    }

    log_info("received tunnel CLOSE: conn=%u public_fd=%d", conn->id, conn->fd);
    connection_unregister(conn, ctx->poll_fds, ctx->connections_by_fd, ctx->connections_by_id);
    log_info("closed public connection after tunnel CLOSE: conn=%u", conn->id);
    connection_destroy(conn);

    return true;
}

bool handle_packet(event_context_t *ctx, const packet_header_t *header) {
    switch (header->type) {
    case PACKET_DATA:
        return handle_data(ctx, header);

    case PACKET_CLOSE:
        return handle_close(ctx, header);

    default:
        log_error("received unknown packet from tunnel client: type=%d conn=%u bytes=%u", header->type,
                  header->connection_id, header->length);
        return false;
    }
}
