#include "server_event.h"
#include "connection.h"
#include "log.h"
#include "protocol.h"
#include "server_packet.h"
#include "vector.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

bool handle_server_events(event_context_t *ctx) {
    struct pollfd server_pfd;
    struct pollfd client_pfd;

    vector_get(ctx->poll_fds, 0, &server_pfd);
    vector_get(ctx->poll_fds, 1, &client_pfd);

    if (!(server_pfd.revents & POLLIN)) return true;

    struct sockaddr_in peer_addr;
    socklen_t peer_addr_len = sizeof(peer_addr);
    int socket_fd = accept(server_pfd.fd, (struct sockaddr *)&peer_addr, &peer_addr_len);
    if (socket_fd == -1) {
        log_errno("failed to accept public socket on fd %d", server_pfd.fd);
        return false;
    }

    char peer_host[INET_ADDRSTRLEN] = "unknown";
    inet_ntop(AF_INET, &peer_addr.sin_addr, peer_host, sizeof(peer_host));
    log_info("accepted public socket: fd=%d peer=%s:%u", socket_fd, peer_host, ntohs(peer_addr.sin_port));

    struct pollfd socket_pfd = {.fd = socket_fd, .events = POLLIN};
    connection_t *conn = connection_create(ctx->server.next_connection_id++, socket_pfd.fd);
    if (conn == NULL) {
        close(socket_fd);
        log_error("failed to create connection for public fd %d", socket_fd);
        return false;
    }

    if (!vector_push(ctx->poll_fds, &socket_pfd)) {
        log_error("failed to track public socket: fd=%d conn=%u", socket_fd, conn->id);
        close(socket_fd);
        connection_destroy(conn);
        return false;
    }

    hashtable_put(ctx->connections_by_fd, &conn->fd, sizeof(int), &conn, sizeof(connection_t *));
    hashtable_put(ctx->connections_by_id, &conn->id, sizeof(uint32_t), &conn, sizeof(connection_t *));

    if (protocol_send_open(client_pfd.fd, conn->id) == -1) {
        log_error("failed to notify tunnel client about new connection: conn=%u public_fd=%d client_fd=%d", conn->id,
                  conn->fd, client_pfd.fd);
        connection_unregister(conn, ctx->poll_fds, ctx->connections_by_fd, ctx->connections_by_id);
        connection_destroy(conn);
        return false;
    }
    log_info("created tunnel connection: conn=%u public_fd=%d client_fd=%d", conn->id, conn->fd, client_pfd.fd);

    return true;
}

bool handle_client_events(event_context_t *ctx) {
    struct pollfd client_pfd;
    vector_get(ctx->poll_fds, 1, &client_pfd);

    if (!(client_pfd.revents & POLLIN)) return true;

    packet_header_t header;
    if (protocol_read_header(client_pfd.fd, &header) == -1) {
        log_error("failed to read packet header from tunnel client fd %d", client_pfd.fd);
        return false;
    }

    log_info("received packet from tunnel client: fd=%d type=%d conn=%u bytes=%u", client_pfd.fd, header.type,
             header.connection_id, header.length);

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
            log_error("missing connection for public socket: fd=%d", socket_pfd.fd);
            continue;
        }

        ssize_t n = read(socket_pfd.fd, conn->read_buffer, sizeof(conn->read_buffer));
        if (n == -1) {
            log_errno("failed to read public socket: conn=%u fd=%d", conn->id, socket_pfd.fd);
            continue;
        }

        if (n == 0) {
            log_info("public socket closed by peer: conn=%u fd=%d", conn->id, conn->fd);
            connection_unregister(conn, ctx->poll_fds, ctx->connections_by_fd, ctx->connections_by_id);

            protocol_send_close(client_pfd.fd, conn->id);
            log_info("closed tunnel connection after public EOF: conn=%u public_fd=%d client_fd=%d", conn->id,
                     socket_pfd.fd, client_pfd.fd);

            connection_destroy(conn);

            continue;
        }

        conn->read_buffer_length = (size_t)n;
        log_info("forwarding public data to tunnel client: conn=%u public_fd=%d client_fd=%d bytes=%zu", conn->id,
                 conn->fd, client_pfd.fd, conn->read_buffer_length);
        if (protocol_send_data(client_pfd.fd, conn->id, conn->read_buffer, conn->read_buffer_length) == -1) {
            log_error("failed to forward public data to tunnel client: conn=%u public_fd=%d client_fd=%d bytes=%zu",
                      conn->id, conn->fd, client_pfd.fd, conn->read_buffer_length);
        }
        conn->read_buffer_length = 0;
    }

    return true;
}
