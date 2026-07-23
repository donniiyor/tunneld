#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <netinet/in.h>
#include <sys/event.h>
#include <sys/socket.h>

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

    if (listen(server_fd, SOMAXCONN) == -1) {
        perror("listen");
        close(server_fd);
        return EXIT_FAILURE;
    }

    printf("Listening on port 8080...\n");

    int kq = kqueue();

    struct kevent change;
    struct kevent events[32];

    EV_SET(&change, server_fd, EVFILT_READ, EV_ADD, 0, 0, NULL);
    kevent(kq, &change, 1, NULL, 0, NULL);

    while (1) {
        int event_count = kevent(kq, NULL, 0, events, 32, NULL);
        for (int i = 0; i < event_count; i++) {
            struct kevent *event = &events[i];

            // New client
            if ((int)event->ident == server_fd) {
                int client_fd = accept(server_fd, NULL, NULL);

                EV_SET(&change, client_fd, EVFILT_READ, EV_ADD, 0, 0, NULL);
                kevent(kq, &change, 1, NULL, 0, NULL);

                printf("Client connected: %d\n", client_fd);

                continue;
            }

            // Existing client
            int client_fd = (int)event->ident;

            char buffer[1024];

            ssize_t n = read(client_fd, buffer, sizeof(buffer));

            if (n == 0) {
                printf("Client disconnected: %d\n", client_fd);

                close(client_fd);

                continue;
            }

            write(client_fd, buffer, n);
        }
    }

    close(server_fd);
    close(kq);

    return EXIT_SUCCESS;
}
