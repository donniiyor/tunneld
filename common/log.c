#include "log.h"

#include <stdarg.h>
#include <stdio.h>
#include <time.h>

static void log_message(const char *level, const char *fmt, va_list args) {
    char timestamp[20];

    time_t now = time(NULL);

    struct tm tm;
    localtime_r(&now, &tm);

    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &tm);

    fprintf(stderr, "[%s] %-5s ", timestamp, level);
    vfprintf(stderr, fmt, args);
    fputc('\n', stderr);
}

void log_info(const char *fmt, ...) {
    va_list args;

    va_start(args, fmt);
    log_message("INFO", fmt, args);
    va_end(args);
}

void log_error(const char *fmt, ...) {
    va_list args;

    va_start(args, fmt);
    log_message("ERROR", fmt, args);
    va_end(args);
}
