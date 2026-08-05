#include "connection.h"
#include "hashtable.h"
#include "log.h"
#include "protocol.h"
#include "server_event.h"
#include "vector.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8080
#define BACKLOG 8
#define KEY_BUFFER_SIZE 12

bool pollfd_cmp(const void *a, const void *b) {
    const struct pollfd *pa = a;
    const int *fd = b;

    return pa->fd == *fd;
}

static int create_listener(uint16_t port) {
    int fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (fd == -1) {
        log_errno("failed to create listener socket: port=%u", port);
        return -1;
    }

    int reuse = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1) {
        log_errno("failed to set SO_REUSEADDR: fd=%d", fd);
        close(fd);
        return -1;
    }

    struct sockaddr_in address = {.sin_family = PF_INET, .sin_addr.s_addr = INADDR_ANY, .sin_port = htons(port)};
    if (bind(fd, (struct sockaddr *)&address, sizeof(address)) == -1) {
        log_errno("failed to bind listener socket: fd=%d port=%u", fd, port);
        close(fd);
        return -1;
    }

    if (listen(fd, BACKLOG) == -1) {
        log_errno("failed to listen on socket: fd=%d backlog=%d", fd, BACKLOG);
        close(fd);
        return -1;
    }

    return fd;
}

static bool get_listener_port(int fd, uint16_t *port) {
    struct sockaddr_in address;
    socklen_t address_len = sizeof(address);

    if (getsockname(fd, (struct sockaddr *)&address, &address_len) == -1) {
        log_errno("failed to get listener address: fd=%d", fd);
        return false;
    }

    *port = ntohs(address.sin_port);
    return true;
}

static bool get_advertised_host(int client_fd, char host[16]) {
    struct sockaddr_in address;
    socklen_t address_len = sizeof(address);

    if (getsockname(client_fd, (struct sockaddr *)&address, &address_len) == -1) {
        log_errno("failed to get tunnel client local address: client_fd=%d", client_fd);
        return false;
    }

    if (inet_ntop(AF_INET, &address.sin_addr, host, 16) == NULL) {
        log_errno("failed to format tunnel endpoint host: client_fd=%d", client_fd);
        return false;
    }

    if (strcmp(host, "0.0.0.0") == 0) {
        strncpy(host, "127.0.0.1", 16);
        host[15] = '\0';
    }

    return true;
}

int main(void) {
    int control_fd = create_listener(PORT);
    if (control_fd == -1) {
        return EXIT_FAILURE;
    }

    log_info("waiting for tunnel client: control_fd=%d port=%d backlog=%d", control_fd, PORT, BACKLOG);

    hashtable_t *connections_by_fd = hashtable_create(sizeof(connection_t) * 10, BACKLOG);
    hashtable_t *connections_by_id = hashtable_create(sizeof(connection_t) * 10, BACKLOG);
    vector_t *poll_fds = vector_create(sizeof(struct pollfd), BACKLOG);

    int client_fd = accept(control_fd, NULL, NULL);
    if (client_fd == -1) {
        log_errno("failed to accept tunnel client on control fd %d", control_fd);
        close(control_fd);
        return EXIT_FAILURE;
    }

    log_info("tunnel client connected: client_fd=%d control_fd=%d", client_fd, control_fd);

    int public_fd = create_listener(0);
    if (public_fd == -1) {
        close(client_fd);
        close(control_fd);
        return EXIT_FAILURE;
    }

    tunnel_endpoint_t endpoint = {0};
    if (!get_listener_port(public_fd, &endpoint.port) || !get_advertised_host(client_fd, endpoint.host)) {
        close(public_fd);
        close(client_fd);
        close(control_fd);
        return EXIT_FAILURE;
    }

    if (protocol_send_tunnel_endpoint(client_fd, &endpoint) == -1) {
        log_error("failed to send allocated tunnel endpoint: client_fd=%d endpoint=%s:%u", client_fd, endpoint.host,
                  endpoint.port);
        close(public_fd);
        close(client_fd);
        close(control_fd);
        return EXIT_FAILURE;
    }

    log_info("allocated tunnel endpoint: client_fd=%d public_fd=%d endpoint=%s:%u", client_fd, public_fd,
             endpoint.host, endpoint.port);

    close(control_fd);

    struct pollfd socket_pfd = {.fd = public_fd, .events = POLLIN};
    vector_push(poll_fds, &socket_pfd);
    struct pollfd client_pfd = {.fd = client_fd, .events = POLLIN};
    vector_push(poll_fds, &client_pfd);

    event_context_t ctx = {.server_fd = public_fd,
                           .poll_fds = poll_fds,
                           .connections_by_fd = connections_by_fd,
                           .connections_by_id = connections_by_id,
                           .localport = PORT};

    while (1) {
        int ready = poll(poll_fds->data, poll_fds->size, -1);
        if (ready == -1) {
            log_errno("poll failed: watched_fds=%zu", poll_fds->size);
            return EXIT_FAILURE;
        }

        if (!handle_server_events(&ctx) || !handle_client_events(&ctx) || !handle_local_events(&ctx)) {
            log_error("event handling failed; shutting down server");
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}
