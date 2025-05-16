#include "common.h"
#include <stdarg.h>
#include <string.h>
#include <time.h>

void get_timestamp(timestamp_t *ts) {
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    ts->ts = now;
    struct tm tm;
    localtime_r(&now.tv_sec, &tm);
    int ms = now.tv_nsec / 1000000;
    snprintf(ts->str, sizeof(ts->str), "%04d-%02d-%02d %02d:%02d:%02d.%03d",
             tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
             tm.tm_hour, tm.tm_min, tm.tm_sec, ms);
}

void log_with_timestamp(const char *fmt, ...) {
    timestamp_t ts;
    get_timestamp(&ts);
    printf("[%s] ", ts.str);
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    printf("\n");
    fflush(stdout);
}