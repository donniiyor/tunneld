#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include <sys/types.h>

enum packet_type {
    PACKET_OPEN = 1,
    PACKET_DATA = 2,
    PACKET_CLOSE = 3,
};

struct packet_header {
    enum packet_type type;
    uint32_t connection_id;
    uint32_t length;
};

ssize_t send_packet(int fd, enum packet_type type, uint32_t connection_id, const void *payload, uint32_t length);

#endif
