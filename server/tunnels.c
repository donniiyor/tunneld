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
        perror("Bind failed");
        close(socket_fd);
        return EXIT_FAILURE;
    }

    if (listen(socket_fd, BACKLOG) == -1) {
        perror("Listen failed");
        close(socket_fd);
        return EXIT_FAILURE;
    }

    printf("Socket is listening on port %d...\n", PORT);

    struct pollfd fds[2];
    fds[0].fd = socket_fd;
    fds[0].events = POLLIN;
    fds[1].fd = -1;

    while (1) {
        int ready = poll(fds, sizeof(fds) / sizeof(fds[0]), -1);

        if (ready == -1) {
            perror("poll");
            break;
        }

        if (fds[0].revents & POLLIN) {
            struct sockaddr_in client_address;
            socklen_t client_address_len = sizeof(client_address);

            int client_fd = accept(socket_fd, (struct sockaddr *)&client_address, &client_address_len);
            if (client_fd == -1) {
                perror("accept");
                continue;
            }

            printf("Client connected\n");

            fds[1].fd = client_fd;
            fds[1].events = POLLIN;

            continue;
        }

        if (fds[1].fd != -1 && (fds[1].revents & POLLIN)) {
            char buffer[1024];
            ssize_t n = recv(fds[1].fd, buffer, sizeof(buffer) - 1, 0);

            if (n <= 0) {
                close(fds[1].fd);
                fds[1].fd = -1;

                printf("Closed connection with client\n");
            } else {
                printf("Received: %s\n", buffer);
                memset(buffer, 0, sizeof(buffer));
            }

            continue;
        }
    }

    return EXIT_SUCCESS;
}
