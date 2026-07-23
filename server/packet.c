#include "connection.h"
#include "protocol.h"

void server_packet_handler(struct connection *conn, struct packet_header *header, uint8_t *payload) {
    switch (header->type) {
    case PACKET_OPEN:
        break;
    case PACKET_DATA:
        break;
    case PACKET_CLOSE:
        break;
    default:
        break;
    }
}
