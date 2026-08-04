#include "connection.h"
#include "hashtable.h"
#include "log.h"
#include "server_event.h"
#include "vector.h"

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

int main(void) {
    int server_fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_fd == -1) {
        log_errno("failed to create server socket");
        return EXIT_FAILURE;
    }

    struct sockaddr_in socket_addres = {.sin_family = PF_INET, .sin_addr.s_addr = INADDR_ANY, .sin_port = htons(PORT)};
    if (bind(server_fd, (struct sockaddr *)&socket_addres, sizeof(socket_addres)) == -1) {
        log_errno("failed to bind server socket: fd=%d port=%d", server_fd, PORT);
        close(server_fd);
        return EXIT_FAILURE;
    }

    if (listen(server_fd, BACKLOG) == -1) {
        log_errno("failed to listen on server socket: fd=%d backlog=%d", server_fd, BACKLOG);
        close(server_fd);
        return EXIT_FAILURE;
    }

    log_info("waiting for tunnel client: listen_fd=%d port=%d backlog=%d", server_fd, PORT, BACKLOG);

    hashtable_t *connections_by_fd = hashtable_create(sizeof(connection_t) * 10, BACKLOG);
    hashtable_t *connections_by_id = hashtable_create(sizeof(connection_t) * 10, BACKLOG);
    vector_t *poll_fds = vector_create(sizeof(struct pollfd), BACKLOG);

    struct pollfd socket_pfd = {.fd = server_fd, .events = POLLIN};
    vector_push(poll_fds, &socket_pfd);

    int client_fd = accept(server_fd, NULL, NULL);
    if (client_fd == -1) {
        log_errno("failed to accept tunnel client on server fd %d", server_fd);
        return EXIT_FAILURE;
    }

    log_info("tunnel client connected: client_fd=%d server_fd=%d", client_fd, server_fd);

    struct pollfd client_pfd = {.fd = client_fd, .events = POLLIN};
    vector_push(poll_fds, &client_pfd);

    event_context_t ctx = {.server_fd = server_fd,
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

        handle_server_events(&ctx);
        handle_client_events(&ctx);
        handle_local_events(&ctx);
    }

    return EXIT_SUCCESS;
}
