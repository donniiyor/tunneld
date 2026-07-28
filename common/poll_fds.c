#include "poll_fds.h"

#include <stddef.h>
#include <string.h>

void fds_add(struct poll_fds *pfds, int fd, short events) {
    pfds->fds[pfds->count].fd = fd;
    pfds->fds[pfds->count].events = events;
    pfds->count++;
}

void fds_del(struct poll_fds *pfds, int fd) {
    size_t elem_index = pfds->count;

    for (size_t i = 0; i < pfds->count; i++) {
        if (pfds->fds[i].fd == fd) {
            elem_index = i;
            break;
        }
    }

    if (elem_index == MAX_CONNECTIONS) return;

    if (elem_index < pfds->count - 1) {
        struct pollfd *src = &pfds->fds[elem_index];
        struct pollfd *dst = &pfds->fds[elem_index + 1];
        size_t move_size = sizeof(struct pollfd) * (pfds->count - (elem_index + 1));

        memmove(src, dst, move_size);
    }

    pfds->count--;
}

struct pollfd *fds_find(struct poll_fds *pfds, int fd) {
    for (size_t i = 0; i < pfds->count; i++) {
        if (pfds->fds[i].fd == fd) return &pfds->fds[i];
    }

    return NULL;
}
