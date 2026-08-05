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

int protocol_send_tunnel_endpoint(int fd, const tunnel_endpoint_t *endpoint) {
    packet_header_t header = {.type = PACKET_TUNNEL_ENDPOINT,
                              .connection_id = 0,
                              .length = sizeof(tunnel_endpoint_t)};

    log_info("sending TUNNEL_ENDPOINT packet: fd=%d endpoint=%s:%u", fd, endpoint->host, endpoint->port);
    if (write_all(fd, &header, sizeof(packet_header_t)) == -1) return -1;
    if (write_all(fd, endpoint, sizeof(tunnel_endpoint_t)) == -1) return -1;

    return 0;
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

int protocol_read_tunnel_endpoint(int fd, tunnel_endpoint_t *endpoint) {
    packet_header_t header;

    if (protocol_read_header(fd, &header) == -1) return -1;

    if (header.type != PACKET_TUNNEL_ENDPOINT || header.length != sizeof(tunnel_endpoint_t)) {
        log_error("expected TUNNEL_ENDPOINT packet: fd=%d type=%d conn=%u bytes=%u", fd, header.type,
                  header.connection_id, header.length);
        return -1;
    }

    if (protocol_read_payload(fd, endpoint, header.length) == -1) return -1;
    endpoint->host[sizeof(endpoint->host) - 1] = '\0';

    log_info("received TUNNEL_ENDPOINT packet: fd=%d endpoint=%s:%u", fd, endpoint->host, endpoint->port);
    return 0;
}
