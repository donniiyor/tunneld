#include "connection.h"

#include <unistd.h>

ssize_t connection_read(struct connection *conn) {
    ssize_t n = read(conn->fd, conn->read_buffer + conn->read_length, BUFFER_SIZE - conn->read_length);

    if (n > 0) conn->read_length += n;

    return n;
}
