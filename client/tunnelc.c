#include "poll_fds.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define SERVER_PORT 8080
#define SERVER_ADDRESS "127.0.0.1"

int main(int argc, char *argv[]) {
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

    struct poll_fds pfds = {.count = 0};
    fds_add(&pfds, server_fd, POLLIN);

    while (1) {
        int ready = poll(pfds.fds, pfds.count, -1);
        if (ready == -1) {
            perror("poll");
            return EXIT_FAILURE;
        }

        // Reaing data flow from server socket
        if (pfds.fds[0].revents & POLLIN) {
            char buffer[1024];

            ssize_t n = read(pfds.fds[0].fd, buffer, sizeof(buffer));
            if (n <= 0) {
                perror("Reading from server socket");

                close(pfds.fds[0].fd);
                printf("Closed server socket connection: %d\n", pfds.fds[0].fd);

                return EXIT_SUCCESS;
            }

            int local_fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
            if (local_fd == -1) {
                perror("Socket creation with local service is failed");
                return EXIT_FAILURE;
            }

            struct sockaddr_in local_address = {.sin_family = AF_INET, .sin_port = htons(atoi(argv[1]))};
            if (inet_pton(AF_INET, SERVER_ADDRESS, &local_address.sin_addr) <= 0) {
                perror("Invalid server address / server address not supported");
                return EXIT_FAILURE;
            }

            if (connect(local_fd, (struct sockaddr *)&local_address, sizeof(local_address)) == -1) {
                perror("Connection attempt with local service is failed");
                return EXIT_FAILURE;
            }

            fds_add(&pfds, local_fd, POLLIN);
            write(pfds.fds[1].fd, buffer, n);

            continue;
        }

        if (pfds.fds[1].revents & POLLIN) {
            char buffer[1024];

            ssize_t n = read(pfds.fds[1].fd, buffer, sizeof(buffer));
            if (n <= 0) {
                perror("read");

                close(pfds.fds[1].fd);
                fds_del(&pfds, pfds.fds[1].fd);

                printf("Closed connection with service: %d\n", pfds.fds[1].fd);

                continue;
            }

            write(pfds.fds[0].fd, buffer, n);
            continue;
        }
    }

    return EXIT_SUCCESS;
}
