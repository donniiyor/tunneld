#ifndef POLL_FDS
#define POLL_FDS

#include <poll.h>
#include <stddef.h>

#define MAX_CONNECTIONS 100

struct poll_fds {
    struct pollfd fds[MAX_CONNECTIONS];
    size_t count;
};

void fds_add(struct poll_fds *pfds, int fd, short events);
void fds_del(struct poll_fds *pfds, int fd);
struct pollfd *fds_find(struct poll_fds *pfds, int fd);

#endif
