#include "client_packet.h"
#include "connection.h"

#include <stdio.h>
#include <unistd.h>

static bool handle_open(event_context_t *ctx, const packet_header_t *header) {
    int local_fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (local_fd == -1) {
        perror("socket creation with local service is failed");
        return false;
    }

    struct sockaddr_in local_address = {.sin_family = AF_INET, .sin_port = htons(ctx->localport)};
    if (inet_pton(AF_INET, ctx->localhost, &local_address.sin_addr) <= 0) {
        perror("invalid server address / server address not supported");
        close(local_fd);
        return false;
    }

    if (connect(local_fd, (struct sockaddr *)&local_address, sizeof(local_address)) == -1) {
        perror("connection attempt with local service is failed");
        close(local_fd);
        return false;
    }

    struct pollfd local_pfd = {.fd = local_fd, .events = POLLIN};
    vector_push(ctx->poll_fds, &local_pfd);

    connection_t *conn = connection_create(header->connection_id, local_fd);

    hashtable_put(ctx->connections_by_id, &header->connection_id, sizeof(uint32_t), &conn, sizeof(connection_t *));
    hashtable_put(ctx->connections_by_fd, &local_fd, sizeof(int), &conn, sizeof(connection_t *));

    printf("connection %d is established\n", conn->id);

    return true;
}

static bool handle_data(event_context_t *ctx, const packet_header_t *header) {
    connection_t *conn = NULL;
    if (!hashtable_get(ctx->connections_by_id, &header->connection_id, sizeof(uint32_t), &conn)) return false;

    protocol_read_payload(ctx->server_fd, conn->write_buffer, header->length);
    conn->write_buffer_length = header->length;

    write(conn->fd, conn->write_buffer, conn->write_buffer_length);
    conn->write_buffer_length = 0;

    return true;
}

static bool handle_close(event_context_t *ctx, const packet_header_t *header) {
    connection_t *conn = NULL;
    if (!hashtable_get(ctx->connections_by_id, &header->connection_id, sizeof(uint32_t), &conn)) return true;

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
        return false;
    }
}
