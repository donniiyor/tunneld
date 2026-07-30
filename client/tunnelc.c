#include "connection.h"
#include "hashtable.h"
#include "protocol.h"
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
        perror("Socket creation failed");
        return EXIT_FAILURE;
    }

    struct sockaddr_in server_address = {.sin_family = AF_INET, .sin_port = htons(SERVER_PORT)};
    if (inet_pton(AF_INET, SERVER_ADDRESS, &server_address.sin_addr) <= 0) {
        perror("Invalid server address / server address not supported");
        return EXIT_FAILURE;
    }

    if (connect(server_fd, (struct sockaddr *)&server_address, sizeof(server_address)) == -1) {
        perror("Connect failed");
        return EXIT_FAILURE;
    }

    printf("Connection with server %d is established\n", server_fd);

    hashtable_t *connections_by_fd = hashtable_create(sizeof(connection_t) * 10, BACKLOG);
    hashtable_t *connections_by_id = hashtable_create(sizeof(connection_t) * 10, BACKLOG);
    vector_t *poll_fds = vector_create(sizeof(struct pollfd), BACKLOG);

    struct pollfd server_pfd = {.fd = server_fd, .events = POLLIN};
    vector_push(poll_fds, &server_pfd);

    while (1) {
        int ready = poll(poll_fds->data, poll_fds->size, -1);
        if (ready == -1) {
            perror("poll");
            return EXIT_FAILURE;
        }

        // Reaing data flow from server socket
        struct pollfd server_pfd;
        vector_get(poll_fds, 0, &server_pfd);
        if (server_pfd.revents & POLLIN) {
            packet_header_t header;
            protocol_read_header(server_pfd.fd, &header);

            if (header.type == PACKET_OPEN) {
                int local_fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
                if (local_fd == -1) {
                    perror("Socket creation with local service is failed");
                    continue;
                }

                struct sockaddr_in local_address = {.sin_family = AF_INET, .sin_port = htons(atoi(argv[1]))};
                if (inet_pton(AF_INET, SERVER_ADDRESS, &local_address.sin_addr) <= 0) {
                    perror("Invalid server address / server address not supported");
                    close(local_fd);
                    continue;
                }

                if (connect(local_fd, (struct sockaddr *)&local_address, sizeof(local_address)) == -1) {
                    perror("Connection attempt with local service is failed");
                    close(local_fd);
                    continue;
                }

                struct pollfd local_pfd = {.fd = local_fd, .events = POLLIN};
                vector_push(poll_fds, &local_pfd);

                connection_t *conn = connection_create(header.connection_id, local_fd);

                hashtable_put(connections_by_id, &header.connection_id, sizeof(uint32_t), &conn, sizeof(connection_t));
                hashtable_put(connections_by_fd, &local_fd, sizeof(int), &conn, sizeof(connection_t));

                printf("Connection %d is established\n", conn->id);

                continue;
            }

            if (header.type == PACKET_DATA) {
                connection_t *conn = NULL;
                if (!hashtable_get(connections_by_id, &header.connection_id, sizeof(uint32_t), &conn)) continue;

                protocol_read_payload(server_pfd.fd, conn->write_buffer, header.length);
                conn->write_buffer_length = header.length;

                write(conn->fd, conn->write_buffer, conn->write_buffer_length);
                conn->write_buffer_length = 0;

                continue;
            }

            if (header.type == PACKET_CLOSE) {
                connection_t *conn = NULL;
                if (!hashtable_get(connections_by_id, &header.connection_id, sizeof(uint32_t), &conn)) continue;

                size_t pollfd_index;
                if (vector_find(poll_fds, &conn->fd, pollfd_cmp, &pollfd_index)) {
                    vector_erase(poll_fds, pollfd_index);
                }

                close(conn->fd);
                hashtable_remove(connections_by_fd, &conn->fd, sizeof(int));
                hashtable_remove(connections_by_id, &conn->id, sizeof(uint32_t));
                free(conn);

                continue;
            }
        }

        for (size_t i = 1; i < poll_fds->size; i++) {
            struct pollfd local_pfd;
            vector_get(poll_fds, i, &local_pfd);

            if (local_pfd.revents & POLLIN) {
                connection_t *conn = NULL;
                if (!hashtable_get(connections_by_fd, &local_pfd.fd, sizeof(int), &conn)) continue;

                ssize_t n = read(local_pfd.fd, conn->read_buffer, sizeof(conn->read_buffer));
                if (n == -1) {
                    perror("read local connection");
                    continue;
                }

                if (n == 0) {
                    close(local_pfd.fd);

                    vector_erase(poll_fds, i--);
                    hashtable_remove(connections_by_fd, &conn->fd, sizeof(int));
                    hashtable_remove(connections_by_id, &conn->id, sizeof(uint32_t));

                    protocol_send_close(server_fd, conn->id);

                    printf("closed local connection: %d\n", conn->id);

                    free(conn);

                    continue;
                }

                conn->read_buffer_length = n;
                protocol_send_data(server_fd, conn->id, conn->read_buffer, conn->read_buffer_length);
                conn->read_buffer_length = 0;
            }
        }
    }

    return EXIT_SUCCESS;
}
