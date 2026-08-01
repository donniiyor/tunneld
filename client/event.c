#include "event.h"
#include "connection.h"
#include "protocol.h"

#include <arpa/inet.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

bool pollfd_cmp(const void *a, const void *b) {
    const struct pollfd *pa = a;
    const int *fd = b;

    return pa->fd == *fd;
}

static void handle_open(event_context_t *ctx, packet_header_t *header) {
    int local_fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (local_fd == -1) {
        perror("socket creation with local service is failed");
        return;
    }

    struct sockaddr_in local_address = {.sin_family = AF_INET, .sin_port = htons(ctx->localport)};
    if (inet_pton(AF_INET, ctx->localhost, &local_address.sin_addr) <= 0) {
        perror("invalid server address / server address not supported");
        close(local_fd);
        return;
    }

    if (connect(local_fd, (struct sockaddr *)&local_address, sizeof(local_address)) == -1) {
        perror("connection attempt with local service is failed");
        close(local_fd);
        return;
    }

    struct pollfd local_pfd = {.fd = local_fd, .events = POLLIN};
    vector_push(ctx->poll_fds, &local_pfd);

    connection_t *conn = connection_create(header->connection_id, local_fd);

    hashtable_put(ctx->connections_by_id, &header->connection_id, sizeof(uint32_t), &conn, sizeof(connection_t *));
    hashtable_put(ctx->connections_by_fd, &local_fd, sizeof(int), &conn, sizeof(connection_t *));

    printf("connection %d is established\n", conn->id);
}

static void handle_data(event_context_t *ctx, packet_header_t *header) {
    connection_t *conn = NULL;
    if (!hashtable_get(ctx->connections_by_id, &header->connection_id, sizeof(uint32_t), &conn)) return;

    protocol_read_payload(ctx->server_fd, conn->write_buffer, header->length);
    conn->write_buffer_length = header->length;

    write(conn->fd, conn->write_buffer, conn->write_buffer_length);
    conn->write_buffer_length = 0;
}

static void handle_close(event_context_t *ctx, packet_header_t *header) {
    connection_t *conn = NULL;
    if (!hashtable_get(ctx->connections_by_id, &header->connection_id, sizeof(uint32_t), &conn)) return;

    size_t pollfd_index;
    if (vector_find(ctx->poll_fds, &conn->fd, pollfd_cmp, &pollfd_index)) {
        vector_erase(ctx->poll_fds, pollfd_index);
    }

    close(conn->fd);
    hashtable_remove(ctx->connections_by_fd, &conn->fd, sizeof(int));
    hashtable_remove(ctx->connections_by_id, &conn->id, sizeof(uint32_t));
    free(conn);
}

bool handle_server_events(event_context_t *ctx) {
    struct pollfd server_pfd;
    vector_get(ctx->poll_fds, 0, &server_pfd);

    if (!(server_pfd.revents & POLLIN)) return true;

    packet_header_t header;
    if (!protocol_read_header(server_pfd.fd, &header)) return false;

    switch (header.type) {
    case PACKET_OPEN:
        handle_open(ctx, &header);
        break;
    case PACKET_DATA:
        handle_data(ctx, &header);
        break;
    case PACKET_CLOSE:
        handle_close(ctx, &header);
        break;
    default:
        fprintf(stderr, "unknown packet type %u\n", header.type);
        break;
    }

    return true;
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
                close(local_pfd.fd);

                vector_erase(ctx->poll_fds, i--);
                hashtable_remove(ctx->connections_by_fd, &conn->fd, sizeof(int));
                hashtable_remove(ctx->connections_by_id, &conn->id, sizeof(uint32_t));

                protocol_send_close(ctx->server_fd, conn->id);

                printf("closed local connection: %d\n", conn->id);

                free(conn);

                continue;
            }

            conn->read_buffer_length = n;
            protocol_send_data(ctx->server_fd, conn->id, conn->read_buffer, conn->read_buffer_length);
            conn->read_buffer_length = 0;
        }
    }

    return true;
}
