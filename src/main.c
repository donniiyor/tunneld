#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

int copy_fd(int src, int dst) {
    char buffer[1024];
    ssize_t n;

    while ((n = read(src, buffer, sizeof(buffer))) > 0) {
        ssize_t total_written = 0;

        while (total_written < n) {
            ssize_t written = write(dst, buffer + total_written, n - total_written); // written could be 0

            if (written == -1) {
                perror("write");
                return -1;
            }

            total_written += written;
        }
    }

    if (n == -1) {
        perror("read");
        return -1;
    }

    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: copy <source> <destination>\n");
        return 1;
    }

    int src = open(argv[1], O_RDONLY);
    if (src == -1) {
        perror("open");
        return 1;
    }

    int dst = open(argv[2], O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (dst == -1) {
        perror("open");
        close(src);
        return 1;
    }

    if (copy_fd(src, dst) == -1) {
        close(src);
        close(dst);
        return -1;
    }

    return 0;
}
