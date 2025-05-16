#define _POSIX_C_SOURCE 200809L
#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <time.h>

// Get current timestamp string with millisecond precision, buf should be at least 64 bytes
typedef struct {
    char str[64];
    struct timespec ts;
} timestamp_t;

void get_timestamp(timestamp_t *ts);
void log_with_timestamp(const char *fmt, ...);

#endif // COMMON_H