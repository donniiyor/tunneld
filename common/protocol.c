#include <string.h>
#include <unistd.h>

#include "protocol.h"

ssize_t send_packet(int fd, enum packet_type type, uint32_t connection_id, const void *payload, uint32_t length) {
    struct packet_header header = {
        .type = type,
        .connection_id = connection_id,
        .length = length,
    };

    if (write(fd, &header, sizeof(header)) != sizeof(header)) return -1;

    if (length > 0) {
        if (write(fd, payload, length) != (ssize_t)length) return -1;
    }

    return sizeof(header) + length;
}
