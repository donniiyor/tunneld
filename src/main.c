#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int copy_fd(int src, int dst) {
    char buffer[1024];

    while (1) {
        ssize_t n = read(src, buffer, sizeof(buffer));

        if (n == -1) {
            perror("read");
            return -1;
        }

        if (n == 0) return 0;

        ssize_t total_written = 0;

        while (total_written < n) {
            ssize_t written = write(dst, buffer + total_written, n - total_written);

            if (written <= 0) {
                perror("write");
                return -1;
            }

            total_written += written;
        }
    }
}

int main(void) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        return EXIT_FAILURE;
    }

    struct sockaddr_in addr = {.sin_family = AF_INET, .sin_port = htons(8080), .sin_addr.s_addr = htonl(INADDR_ANY)};

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("bind");
        close(server_fd);
        return EXIT_FAILURE;
    }

    if (listen(server_fd, 10) == -1) {
        perror("listen");
        close(server_fd);
        return EXIT_FAILURE;
    }

    printf("Listening on port 8080...\n");

    int client_fd = accept(server_fd, NULL, NULL);
    if (client_fd == -1) {
        perror("accept");
        close(server_fd);
        return EXIT_FAILURE;
    }

    printf("Client connected\n");

    if (copy_fd(client_fd, STDOUT_FILENO) == -1) {
        close(server_fd);
        close(client_fd);
        return EXIT_FAILURE;
    }

    close(server_fd);
    close(client_fd);

    return EXIT_SUCCESS;
}
