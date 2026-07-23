#ifndef CONNECTION_H
#define CONNECTION_H

#include "protocol.h"
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#define BUFFER_SIZE 65536

struct connection;

typedef void (*packet_handler_fn)(struct connection *conn, struct packet_header *header, uint8_t *payload);

struct connection {
    int fd;

    uint8_t read_buffer[BUFFER_SIZE];
    size_t read_length;

    uint8_t write_buffer[BUFFER_SIZE];
    size_t write_length;

    packet_handler_fn packet_handler;
};

ssize_t connection_read(struct connection *conn);
void connection_parse(struct connection *conn);

#endif
