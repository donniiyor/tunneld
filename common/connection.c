#include "connection.h"

#include <stdio.h>
#include <stdlib.h>

connection_t *connection_create(uint32_t id, int fd) {
    connection_t *conn = malloc(sizeof(connection_t));
    if (conn == NULL) {
        perror("malloc");
        return NULL;
    }

    conn->id = id;
    conn->fd = fd;

    conn->read_buffer_length = 0;
    conn->write_buffer_length = 0;
    conn->state = OPEN;

    return conn;
}
