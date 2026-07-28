#include "poll_fds.h"

#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8080
#define BACKLOG 8

int main(void) {
    int socket_fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socket_fd == -1) {
        perror("Socket creation failed");
        return EXIT_FAILURE;
    }

    struct sockaddr_in address = {.sin_family = PF_INET, .sin_addr.s_addr = INADDR_ANY, .sin_port = htons(PORT)};
    if (bind(socket_fd, (struct sockaddr *)&address, sizeof(address)) == -1) {
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

    struct poll_fds pfds = {.count = 0};
    fds_add(&pfds, socket_fd, POLLIN);

    int client_fd = accept(pfds.fds[0].fd, NULL, NULL);
    if (client_fd == -1) {
        perror("Failed on client socket accept");
        return EXIT_FAILURE;
    }

    printf("Client is connected: %d\n", client_fd);
    fds_add(&pfds, client_fd, POLLIN);

    while (1) {
        int ready = poll(pfds.fds, pfds.count, -1);
        if (ready == -1) {
            perror("poll");
            return EXIT_FAILURE;
        }

        // Check for user connections on socket_fd
        if (pfds.fds[0].revents & POLLIN) {
            int user_fd = accept(pfds.fds[0].fd, NULL, NULL);
            if (client_fd == -1) {
                perror("Failed on user socket accept");
                continue;
            }

            fds_add(&pfds, user_fd, POLLIN);
            printf("Accepted user socket: %d\n", user_fd);

            continue;
        }

        // Reading data flow from client socket
        if (pfds.fds[1].revents & POLLIN) {
            char buffer[1024];

            ssize_t n = read(pfds.fds[1].fd, buffer, sizeof(buffer));
            if (n <= 0) {
                perror("Reading from client_fd");

                fds_del(&pfds, pfds.fds[1].fd);
                close(pfds.fds[1].fd);

                return EXIT_FAILURE;
            }

            // Broadcasting data flow to all user sockets
            for (size_t i = 2; i < pfds.count; i++) {
                write(pfds.fds[i].fd, buffer, n);
            }
        }

        // Reading from user_fd and writing to client_fd
        for (size_t i = 2; i < pfds.count; i++) {
            if (pfds.fds[i].revents & POLLIN) {
                char buffer[1024];

                ssize_t n = read(pfds.fds[i].fd, buffer, sizeof(buffer));
                if (n <= 0) {
                    perror("Reading from user sockets");

                    fds_del(&pfds, pfds.fds[i].fd);
                    close(pfds.fds[i].fd);

                    printf("Closed user socket connection: %d\n", pfds.fds[i].fd);
                    continue;
                }

                // Write data flow to client socket
                write(pfds.fds[1].fd, buffer, n);
            }
        }
    }

    return EXIT_SUCCESS;
}
