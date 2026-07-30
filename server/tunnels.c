#include "connection.h"
#include "hashtable.h"
#include "protocol.h"
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

typedef struct {
    uint32_t next_connection_id;
} server_t;

bool pollfd_cmp(const void *a, const void *b) {
    const struct pollfd *pa = a;
    const struct pollfd *pb = b;

    return pa->fd == pb->fd;
}

int main(void) {
    int socket_fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socket_fd == -1) {
        perror("Socket creation failed");
        return EXIT_FAILURE;
    }

    struct sockaddr_in socket_addres = {.sin_family = PF_INET, .sin_addr.s_addr = INADDR_ANY, .sin_port = htons(PORT)};
    if (bind(socket_fd, (struct sockaddr *)&socket_addres, sizeof(socket_addres)) == -1) {
        perror("Socket address bind failed");
        close(socket_fd);
        return EXIT_FAILURE;
    }

    if (listen(socket_fd, BACKLOG) == -1) {
        perror("Socket listen failed");
        close(socket_fd);
        return EXIT_FAILURE;
    }

    printf("Socket is wating for client connection on port %d...\n", PORT);

    hashtable_t *connections_by_fd = hashtable_create(sizeof(connection_t) * 10, BACKLOG);
    hashtable_t *connections_by_id = hashtable_create(sizeof(connection_t) * 10, BACKLOG);
    vector_t *poll_fds = vector_create(sizeof(struct pollfd), BACKLOG);
    server_t server = {.next_connection_id = 0};

    struct pollfd socket_pfd = {.fd = socket_fd, .events = POLLIN};
    vector_push(poll_fds, &socket_pfd);

    int client_fd = accept(socket_fd, NULL, NULL);
    if (client_fd == -1) {
        perror("Failed on client socket accept");
        return EXIT_FAILURE;
    }

    printf("Client is connected: %d\n", client_fd);

    struct pollfd client_pfd = {.fd = client_fd, .events = POLLIN};
    vector_push(poll_fds, &client_pfd);

    while (1) {
        int ready = poll(poll_fds->data, poll_fds->size, -1);
        if (ready == -1) {
            perror("poll");
            return EXIT_FAILURE;
        }

        // Check for user connections on socket_fd
        if (socket_pfd.revents & POLLIN) {
            int user_fd = accept(socket_pfd.fd, NULL, NULL);
            if (client_fd == -1) {
                perror("Failed on user socket accept");
                continue;
            }

            printf("Accepted user socket: %d\n", user_fd);

            struct pollfd *user_pfd = malloc(sizeof(struct pollfd));
            user_pfd->fd = user_fd;
            user_pfd->events = POLLIN;

            vector_push(poll_fds, user_pfd);

            connection_t *conn = connection_create(server.next_connection_id++, user_fd);

            hashtable_put(connections_by_fd, &user_fd, sizeof(user_fd), conn, sizeof(connection_t));
            hashtable_put(connections_by_id, &conn->id, sizeof(conn->id), conn, sizeof(connection_t));

            protocol_send_open(client_fd, conn->id);

            printf("Connection created: %d\n", conn->id);

            continue;
        }

        // Reading data flow from client socket
        if (client_pfd.revents & POLLIN) {
            packet_header_t header;
            protocol_read_header(client_pfd.fd, &header);

            connection_t *conn;
            hashtable_get(connections_by_id, &header.connection_id, sizeof(uint32_t), &conn);

            if (header.type == PACKET_DATA) {
                protocol_read_payload(client_pfd.fd, conn->write_buffer, header.length);
                write(conn->fd, conn->write_buffer, conn->write_buffer_length);
                conn->write_buffer_length = 0;
            }

            if (header.type == PACKET_CLOSE) {
                hashtable_remove(connections_by_id, &conn->id, sizeof(uint32_t));
                hashtable_remove(connections_by_fd, &conn->fd, sizeof(int));

                size_t pollfd_index;
                vector_find(poll_fds, &conn->fd, pollfd_cmp, &pollfd_index);

                struct pollfd *pfd;
                vector_get(poll_fds, pollfd_index, &pfd);
                vector_erase(poll_fds, pollfd_index);

                close(conn->fd);

                free(conn);
                free(pfd);
            }
        }

        // Reading data from user fds
        for (size_t i = 2; i < poll_fds->size; i++) {
            struct pollfd *user_pfd;
            vector_get(poll_fds, i, &user_pfd);

            if (user_pfd->revents & POLLIN) {
                connection_t *conn;
                hashtable_get(connections_by_fd, &user_pfd->fd, sizeof(user_pfd->fd), conn);

                ssize_t n = read(user_pfd->fd, conn->read_buffer, sizeof(conn->read_buffer));
                if (n == 0) {
                    close(user_pfd->fd);

                    vector_erase(poll_fds, i);
                    hashtable_remove(connections_by_fd, &user_pfd->fd, sizeof(int));
                    hashtable_remove(connections_by_id, &conn->id, sizeof(uint32_t));

                    protocol_send_close(client_fd, conn->id);

                    printf("Closed user socket connection: %d\n", user_pfd->fd);

                    free(conn);
                    free(user_pfd);

                    continue;
                }

                // Write data flow to client socket
                protocol_send_data(client_fd, conn->id, conn->read_buffer, conn->read_buffer_length);
                conn->read_buffer_length = 0;
            }
        }
    }

    return EXIT_SUCCESS;
}
