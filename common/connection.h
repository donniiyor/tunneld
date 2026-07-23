#ifndef CONNECTION_H
#define CONNECTION_H

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#define BUFFER_SIZE 65536

struct connection {
    int fd;

    uint8_t read_buffer[BUFFER_SIZE];
    size_t read_length;

    uint8_t write_buffer[BUFFER_SIZE];
    size_t write_length;
};

ssize_t connection_read(struct connection *conn);

#endif
