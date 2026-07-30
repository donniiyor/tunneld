#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

enum packet_type {
    PACKET_OPEN = 1,
    PACKET_DATA = 2,
    PACKET_CLOSE = 3,
};

typedef struct {
    enum packet_type type;
    uint32_t connection_id;
    uint32_t length;
} packet_header_t;

int protocol_send_open(int fd, uint32_t connection_id);
int protocol_send_data(int fd, uint32_t connection_id, const void *payload, uint32_t length);
int protocol_send_close(int fd, uint32_t connection_id);

int protocol_read_header(int fd, packet_header_t *header);
int protocol_read_payload(int fd, void *payload, uint32_t length);

#endif
