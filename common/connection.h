#ifndef CONNECTION_MAP
#define CONNECTION_MAP

#include <stddef.h>
#include <stdint.h>

#define BUFFER_SIZE 4096

enum connection_state {
    OPEN,
    CLOSING,
};

typedef struct {
    uint32_t id;

    int fd;

    char read_buffer[BUFFER_SIZE];
    size_t read_buffer_length;

    char write_buffer[BUFFER_SIZE];
    size_t write_buffer_length;

    enum connection_state state;
} connection_t;

connection_t *connection_create(uint32_t id, int fd);

#endif
