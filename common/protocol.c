#include "protocol.h"

#include <stdio.h>
#include <unistd.h>

int protocol_send_open(int fd, uint32_t connection_id) {
    packet_header_t header = {.type = PACKET_OPEN, .connection_id = connection_id, .length = 0};

    write(fd, &header, sizeof(packet_header_t));

    return 0;
}

int protocol_send_data(int fd, uint32_t connection_id, const void *payload, uint32_t length) {
    packet_header_t header = {.type = PACKET_DATA, .connection_id = connection_id, .length = length};

    write(fd, &header, sizeof(packet_header_t));
    write(fd, payload, length);

    return 0;
}

int protocol_send_close(int fd, uint32_t connection_id) {
    packet_header_t header = {.type = PACKET_CLOSE, .connection_id = connection_id, .length = 0};

    write(fd, &header, sizeof(packet_header_t));

    return 0;
}

int protocol_read_header(int fd, packet_header_t *header) {
    ssize_t n = read(fd, header, sizeof(packet_header_t));
    if (n == -1) {
        perror("Reading packet header");
        return -1;
    }

    return 0;
}

int protocol_read_payload(int fd, void *payload, uint32_t length) {
    ssize_t n = read(fd, payload, length);
    if (n == -1) {
        perror("Reading packet payload");
        return -1;
    }

    return 0;
}
