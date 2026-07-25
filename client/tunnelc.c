#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
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

    return EXIT_SUCCESS;
}
