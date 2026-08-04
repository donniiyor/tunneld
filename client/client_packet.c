#include "client_packet.h"
#include "connection.h"
#include "log.h"

#include <stdio.h>
#include <unistd.h>

static bool handle_open(event_context_t *ctx, const packet_header_t *header) {
    int local_fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (local_fd == -1) {
        log_errno("failed to create local service socket: conn=%u", header->connection_id);
        return false;
    }

    struct sockaddr_in local_address = {.sin_family = AF_INET, .sin_port = htons(ctx->localport)};
    if (inet_pton(AF_INET, ctx->localhost, &local_address.sin_addr) <= 0) {
        log_error("invalid local service address: conn=%u address=%s", header->connection_id, ctx->localhost);
        close(local_fd);
        return false;
    }

    if (connect(local_fd, (struct sockaddr *)&local_address, sizeof(local_address)) == -1) {
        log_errno("failed to connect to local service: conn=%u fd=%d target=%s:%u", header->connection_id, local_fd,
                  ctx->localhost, ctx->localport);
        close(local_fd);
        return false;
    }

    connection_t *conn = connection_create(header->connection_id, local_fd);
    if (conn == NULL) {
        log_error("failed to create local connection: conn=%u fd=%d", header->connection_id, local_fd);
        close(local_fd);
        return false;
    }

    struct pollfd local_pfd = {.fd = local_fd, .events = POLLIN};
    if (!vector_push(ctx->poll_fds, &local_pfd)) {
        log_error("failed to track local service socket: conn=%u fd=%d", header->connection_id, local_fd);
        close(local_fd);
        connection_destroy(conn);
        return false;
    }

    hashtable_put(ctx->connections_by_id, &header->connection_id, sizeof(uint32_t), &conn, sizeof(connection_t *));
    hashtable_put(ctx->connections_by_fd, &local_fd, sizeof(int), &conn, sizeof(connection_t *));

    log_info("opened local service connection: conn=%u local_fd=%d target=%s:%u", conn->id, local_fd, ctx->localhost,
             ctx->localport);

    return true;
}

static bool handle_data(event_context_t *ctx, const packet_header_t *header) {
    connection_t *conn = NULL;
    if (!hashtable_get(ctx->connections_by_id, &header->connection_id, sizeof(uint32_t), &conn)) {
        log_error("received DATA for unknown local connection: conn=%u bytes=%u", header->connection_id,
                  header->length);
        return false;
    }

    if (header->length > sizeof(conn->write_buffer)) {
        log_error("packet payload too large from tunnel server: conn=%u bytes=%u limit=%zu", header->connection_id,
                  header->length, sizeof(conn->write_buffer));
        return false;
    }

    if (protocol_read_payload(ctx->server_fd, conn->write_buffer, header->length) == -1) {
        log_error("failed to read DATA payload from tunnel server: conn=%u server_fd=%d bytes=%u", conn->id,
                  ctx->server_fd, header->length);
        return false;
    }

    conn->write_buffer_length = header->length;

    log_info("forwarding tunnel data to local service: conn=%u local_fd=%d server_fd=%d bytes=%zu", conn->id,
             conn->fd, ctx->server_fd, conn->write_buffer_length);
    ssize_t n = write(conn->fd, conn->write_buffer, conn->write_buffer_length);
    if (n == -1) {
        log_errno("failed to write tunnel data to local service: conn=%u local_fd=%d bytes=%zu", conn->id, conn->fd,
                  conn->write_buffer_length);
        return false;
    }

    if ((size_t)n != conn->write_buffer_length) {
        log_error("short write to local service: conn=%u local_fd=%d wrote=%zd expected=%zu", conn->id, conn->fd, n,
                  conn->write_buffer_length);
    }
    conn->write_buffer_length = 0;

    return true;
}

static bool handle_close(event_context_t *ctx, const packet_header_t *header) {
    connection_t *conn = NULL;
    if (!hashtable_get(ctx->connections_by_id, &header->connection_id, sizeof(uint32_t), &conn)) {
        log_info("received CLOSE for unknown local connection: conn=%u", header->connection_id);
        return true;
    }

    log_info("received tunnel CLOSE: conn=%u local_fd=%d", conn->id, conn->fd);
    connection_unregister(conn, ctx->poll_fds, ctx->connections_by_fd, ctx->connections_by_id);
    connection_destroy(conn);

    return true;
}

bool handle_packet(event_context_t *ctx, const packet_header_t *header) {
    switch (header->type) {
    case PACKET_OPEN:
        return handle_open(ctx, header);

    case PACKET_DATA:
        return handle_data(ctx, header);

    case PACKET_CLOSE:
        return handle_close(ctx, header);

    default:
        log_error("received unknown packet from tunnel server: type=%d conn=%u bytes=%u", header->type,
                  header->connection_id, header->length);
        return false;
    }
}
