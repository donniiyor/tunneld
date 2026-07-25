#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8080

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

    if (listen(socket_fd, 10) == -1) {
        perror("Listen failed");
        close(socket_fd);
        return EXIT_FAILURE;
    }

    printf("Socket is listening on port %d...\n", PORT);

    struct sockaddr_in client_address;
    socklen_t client_address_len = sizeof(client_address);
    int client_fd = accept(socket_fd, (struct sockaddr *)&client_address, &client_address_len);

    printf("Client has connected %d\n", client_fd);

    return EXIT_SUCCESS;
}
