#include "connection.h"
#include "event.h"
#include "hashtable.h"
#include "vector.h"

#include <arpa/inet.h>
#include <assert.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BACKLOG 8
#define SERVER_PORT 8080
#define SERVER_ADDRESS "127.0.0.1"

bool pollfd_cmp(const void *a, const void *b) {
    const struct pollfd *pa = a;
    const int *fd = b;

    return pa->fd == *fd;
}

int main(int argc, char *argv[]) {
    assert(argc == 2);

    // Establish connection with server
    int server_fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_fd == -1) {
        perror("socket creation failed");
        return EXIT_FAILURE;
    }

    struct sockaddr_in server_address = {.sin_family = AF_INET, .sin_port = htons(SERVER_PORT)};
    if (inet_pton(AF_INET, SERVER_ADDRESS, &server_address.sin_addr) <= 0) {
        perror("invalid server address / server address not supported");
        return EXIT_FAILURE;
    }

    if (connect(server_fd, (struct sockaddr *)&server_address, sizeof(server_address)) == -1) {
        perror("connect failed");
        return EXIT_FAILURE;
    }

    printf("connection with server %d is established\n", server_fd);

    hashtable_t *connections_by_fd = hashtable_create(sizeof(connection_t) * 10, BACKLOG);
    hashtable_t *connections_by_id = hashtable_create(sizeof(connection_t) * 10, BACKLOG);
    vector_t *poll_fds = vector_create(sizeof(struct pollfd), BACKLOG);

    struct pollfd server_pfd = {.fd = server_fd, .events = POLLIN};
    vector_push(poll_fds, &server_pfd);

    event_context_t ctx = {.server_fd = server_fd,
                           .poll_fds = poll_fds,
                           .connections_by_fd = connections_by_fd,
                           .connections_by_id = connections_by_id,
                           .localhost = SERVER_ADDRESS,
                           .localport = SERVER_PORT};

    while (1) {
        int ready = poll(poll_fds->data, poll_fds->size, -1);
        if (ready == -1) {
            perror("poll");
            return EXIT_FAILURE;
        }

        handle_server_events(&ctx);
        handle_local_events(&ctx);
    }

    return EXIT_SUCCESS;
}
