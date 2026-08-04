#include "protocol.h"
#include "log.h"

#include <errno.h>
#include <stddef.h>
#include <unistd.h>

static int write_all(int fd, const void *buf, size_t length) {
    const char *p = buf;

    while (length > 0) {
        ssize_t n = write(fd, p, length);
        if (n == -1) {
            if (errno == EINTR) continue;
            log_errno("failed to write %zu bytes to fd %d", length, fd);
            return -1;
        }

        p += n;
        length -= (size_t)n;
    }

    return 0;
}

static int read_all(int fd, void *buf, size_t length) {
    char *p = buf;

    while (length > 0) {
        ssize_t n = read(fd, p, length);
        if (n == -1) {
            if (errno == EINTR) continue;
            log_errno("failed to read %zu bytes from fd %d", length, fd);
            return -1;
        }

        if (n == 0) {
            log_error("unexpected EOF while reading %zu bytes from fd %d", length, fd);
            return -1;
        }

        p += n;
        length -= (size_t)n;
    }

    return 0;
}

int protocol_send_open(int fd, uint32_t connection_id) {
    packet_header_t header = {.type = PACKET_OPEN, .connection_id = connection_id, .length = 0};

    log_info("sending OPEN packet: fd=%d conn=%u", fd, connection_id);
    return write_all(fd, &header, sizeof(packet_header_t));
}

int protocol_send_data(int fd, uint32_t connection_id, const void *payload, uint32_t length) {
    packet_header_t header = {.type = PACKET_DATA, .connection_id = connection_id, .length = length};

    log_info("sending DATA packet: fd=%d conn=%u bytes=%u", fd, connection_id, length);
    if (write_all(fd, &header, sizeof(packet_header_t)) == -1) return -1;
    if (write_all(fd, payload, length) == -1) return -1;

    return 0;
}

int protocol_send_close(int fd, uint32_t connection_id) {
    packet_header_t header = {.type = PACKET_CLOSE, .connection_id = connection_id, .length = 0};

    log_info("sending CLOSE packet: fd=%d conn=%u", fd, connection_id);
    return write_all(fd, &header, sizeof(packet_header_t));
}

int protocol_read_header(int fd, packet_header_t *header) {
    return read_all(fd, header, sizeof(packet_header_t));
}

int protocol_read_payload(int fd, void *payload, uint32_t length) {
    return read_all(fd, payload, length);
}
