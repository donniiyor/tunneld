#include "connection.h"
#include "protocol.h"

void client_packet_handler(struct connection *conn, struct packet_header *header, uint8_t *payload) {
    switch (header->type) {
    case PACKET_OPEN:
        // connect(localhost)
        break;

    case PACKET_DATA:
        // write(local_fd)
        break;

    case PACKET_CLOSE:
        // close(local_fd)
        break;
    }
}
