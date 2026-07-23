#include "connection.h"
#include "protocol.h"

#include <stdint.h>
#include <string.h>
#include <unistd.h>

ssize_t connection_read(struct connection *conn) {
    ssize_t n = read(conn->fd, conn->read_buffer + conn->read_length, BUFFER_SIZE - conn->read_length);

    if (n > 0) conn->read_length += n;

    return n;
}

void connection_parse(struct connection *conn) {
    while (1) {
        if (conn->read_length < sizeof(struct packet_header)) return;

        struct packet_header *header = (struct packet_header *)conn->read_buffer;

        size_t packet_size = sizeof(struct packet_header) + header->length;

        if (conn->read_length < packet_size) return;

        uint8_t *payload = conn->read_buffer + sizeof(struct packet_header);

        conn->packet_handler(conn, header, payload);

        memmove(conn->read_buffer, conn->read_buffer + packet_size, conn->read_length - packet_size);

        conn->read_length -= packet_size;
    }
}
