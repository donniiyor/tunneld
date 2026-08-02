#include "connection.h"
#include "hashtable.h"

#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static bool pollfd_cmp(const void *a, const void *b) {
    const struct pollfd *pa = a;
    const int *fd = b;

    return pa->fd == *fd;
}

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

void connection_unregister(connection_t *conn, vector_t *poll_fds, hashtable_t *connections_by_fd, hashtable_t *connections_by_id) {
    size_t index;
    if (vector_find(poll_fds, &conn->fd, pollfd_cmp, &index)) vector_erase(poll_fds, index);

    hashtable_remove(connections_by_fd, &conn->fd, sizeof(conn->fd));
    hashtable_remove(connections_by_id, &conn->id, sizeof(conn->id));

    close(conn->fd);
}

void connection_destroy(connection_t *conn) {
    free(conn);
}
