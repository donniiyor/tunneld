#include "server_packet.h"
#include "connection.h"
#include "log.h"
#include "protocol.h"
#include "server_event.h"
#include "vector.h"

#include <stdio.h>
#include <unistd.h>

static bool handle_data(event_context_t *ctx, const packet_header_t *header) {
    connection_t *conn = NULL;
    if (!hashtable_get(ctx->connections_by_id, &header->connection_id, sizeof(uint32_t), &conn)) {
        log_error("missing connection %d", header->connection_id);
        return false;
    }

    struct pollfd client_pfd;
    vector_get(ctx->poll_fds, 1, &client_pfd);

    conn->write_buffer_length = header->length;
    write(conn->fd, conn->write_buffer, conn->write_buffer_length);
    conn->write_buffer_length = 0;

    return true;
}

static bool handle_close(event_context_t *ctx, const packet_header_t *header) {
    connection_t *conn = NULL;
    if (!hashtable_get(ctx->connections_by_id, &header->connection_id, sizeof(uint32_t), &conn)) {
        log_error("missing connection %d", header->connection_id);
        return false;
    }

    connection_unregister(conn, ctx->poll_fds, ctx->connections_by_fd, ctx->connections_by_id);
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
        return false;
    }
}
