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

int main(void) {
    int socket_fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socket_fd == -1) {
        perror("Socket creation failed");
        return EXIT_FAILURE;
    }

    struct sockaddr_in server_address = {.sin_family = AF_INET, .sin_port = htons(SERVER_PORT)};
    if (inet_pton(AF_INET, SERVER_ADDRESS, &server_address.sin_addr) <= 0) {
        perror("Invalid server address / server address not supported");
        return EXIT_FAILURE;
    }

    if (connect(socket_fd, (struct sockaddr *)&server_address, sizeof(server_address)) == -1) {
        perror("Connect failed");
        return EXIT_FAILURE;
    }

    struct pollfd fds[2];
    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN;
    fds[1].fd = socket_fd;
    fds[1].events = POLLIN;

    while (1) {
        int ready = poll(fds, sizeof(fds) / sizeof(fds[0]), -1);

        if (ready == -1) {
            perror("poll");
            break;
        }

        if (fds[0].events & POLLIN) {
            char buffer[1024];
            ssize_t n = read(fds[0].fd, buffer, sizeof(buffer));

            if (n <= 0) {
                perror("read");
                continue;
            }

            send(fds[1].fd, buffer, n, 0);
            memset(buffer, 0, sizeof(buffer));

            continue;
        }
    }

    return EXIT_SUCCESS;
}
