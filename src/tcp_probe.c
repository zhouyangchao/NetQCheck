#define _POSIX_C_SOURCE 200809L
#include <unistd.h>
#include "common.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/time.h>
#include <getopt.h>
#include <math.h>

#define DEFAULT_FREQ 1.0 // Hz
#define MIN_TIMEOUT_MS 100
#define MAX_TIMEOUT_MS 5000

static int set_nonblock(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

static int tcp_connect_probe(const char *ip, int port, int timeout_ms, int *err_ret) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) { if (err_ret) *err_ret = errno; return -1; }
    set_nonblock(sock);
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &addr.sin_addr);
    int ret = connect(sock, (struct sockaddr*)&addr, sizeof(addr));
    if (ret == 0) { close(sock); if (err_ret) *err_ret = 0; return 0; }
    if (errno != EINPROGRESS) { if (err_ret) *err_ret = errno; close(sock); return -1; }
    fd_set wfds;
    FD_ZERO(&wfds);
    FD_SET(sock, &wfds);
    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    ret = select(sock+1, NULL, &wfds, NULL, &tv);
    int err = 0; socklen_t len = sizeof(err);
    if (ret > 0 && FD_ISSET(sock, &wfds)) {
        getsockopt(sock, SOL_SOCKET, SO_ERROR, &err, &len);
        close(sock);
        if (err_ret) *err_ret = err;
        return err == 0 ? 0 : -1;
    }
    if (err_ret) *err_ret = (ret == 0) ? ETIMEDOUT : errno;
    close(sock);
    return -1;
}

void usage(const char *prog, int code) {
    printf("Usage: %s [-f <freq Hz>] [-d <duration s>] [-q] [--help] <host> [port]\n", prog);
    printf("  <host>           Target host (IP or domain)\n");
    printf("  [port]           Target port (default: 443)\n");
    printf("  -f, --freq       Probe frequency in Hz (default: 1.0)\n");
    printf("  -d, --duration   Probe duration in seconds (default: unlimited)\n");
    printf("  -q, --quiet      Only print summary statistics\n");
    printf("  -h, --help       Show this help message\n");
    exit(code);
}

int main(int argc, char *argv[]) {
    int port = 443;
    double freq = DEFAULT_FREQ;
    int duration = 0;
    int opt;
    int option_index = 0;
    int quiet = 0;
    static struct option long_options[] = {
        {"freq",     required_argument, 0, 'f'},
        {"duration", required_argument, 0, 'd'},
        {"quiet",    no_argument,       0, 'q'},
        {"help",     no_argument,       0, 'h'},
        {0, 0, 0, 0}
    };
    while ((opt = getopt_long(argc, argv, "f:d:qh", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'f': freq = atof(optarg); break;
            case 'd': duration = atoi(optarg); break;
            case 'q': quiet = 1; break;
            case 'h': usage(argv[0], 0); break;
            default: usage(argv[0], 1);
        }
    }
    if (optind >= argc) {
        fprintf(stderr, "Error: host is required.\n");
        usage(argv[0], 1);
    }
    const char *ip = argv[optind];
    if (optind + 1 < argc) {
        port = atoi(argv[optind + 1]);
    }
    if (freq <= 0) freq = DEFAULT_FREQ;
    int timeout_ms = (int)(1000.0 / freq * 0.8);
    if (timeout_ms < MIN_TIMEOUT_MS) timeout_ms = MIN_TIMEOUT_MS;
    if (timeout_ms > MAX_TIMEOUT_MS) timeout_ms = MAX_TIMEOUT_MS;
    int interval_ms = (int)(1000.0 / freq);

    int probe_count = 0, recv_count = 0;
    double up_time = 0, down_time = 0;
    struct timespec start, now;
    int N = duration > 0 ? (int)(duration * freq) : -1;
    int i = 0;
    double *rtt_list = NULL;
    int rtt_cap = N > 0 ? N : 10000;
    rtt_list = calloc(rtt_cap, sizeof(double));
    if (!rtt_list) { perror("calloc"); exit(2); }
    printf("TCPING %s:%d with %.2f probes per second\n", ip, port, freq);
    clock_gettime(CLOCK_MONOTONIC, &start);
    struct timespec target = start;
    for (i = 0; N < 0 || i < N; ++i) {
        struct timespec t1, t2;
        clock_gettime(CLOCK_MONOTONIC, &t1);
        int err = 0;
        int status = tcp_connect_probe(ip, port, timeout_ms, &err);
        clock_gettime(CLOCK_MONOTONIC, &t2);
        double rtt = (t2.tv_sec - t1.tv_sec) * 1000.0 + (t2.tv_nsec - t1.tv_nsec) / 1e6;
        clock_gettime(CLOCK_MONOTONIC, &now);
        probe_count++;
        if (status == 0) {
            recv_count++;
            if (recv_count <= rtt_cap) rtt_list[recv_count-1] = rtt;
            if (!quiet) printf("tcp_seq=%d time=%.3f ms Connected\n", i+1, rtt);
            up_time += interval_ms / 1000.0;
        } else {
            if (!quiet) printf("tcp_seq=%d time=%.3f ms Can't Connected (%s)\n", i+1, rtt, strerror(err));
            down_time += interval_ms / 1000.0;
        }
        // Calculate next cycle's target time
        target.tv_nsec += interval_ms * 1000000;
        while (target.tv_nsec >= 1000000000) {
            target.tv_sec += 1;
            target.tv_nsec -= 1000000000;
        }
        struct timespec now2;
        clock_gettime(CLOCK_MONOTONIC, &now2);
        time_t sec = target.tv_sec - now2.tv_sec;
        long nsec = target.tv_nsec - now2.tv_nsec;
        if (nsec < 0) { sec -= 1; nsec += 1000000000; }
        if (sec > 0 || (sec == 0 && nsec > 0)) {
            struct timespec ts_sleep = { sec, nsec };
            nanosleep(&ts_sleep, NULL);
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &now);
    double total_time = (now.tv_sec - start.tv_sec) * 1000.0 + (now.tv_nsec - start.tv_nsec) / 1e6;
    // 统计rtt min/avg/max/mdev
    double min=0, max=0, sum=0, sum2=0, avg=0, mdev=0;
    if (recv_count > 0) {
        min = max = rtt_list[0];
        for (int j=0; j<recv_count; ++j) {
            double v = rtt_list[j];
            if (v < min) min = v;
            if (v > max) max = v;
            sum += v;
        }
        avg = sum / recv_count;
        for (int j=0; j<recv_count; ++j) {
            double d = fabs(rtt_list[j] - avg);
            sum2 += d * d;
        }
        mdev = sqrt(sum2 / recv_count);
    }
    double loss = probe_count > 0 ? 100.0 * (probe_count - recv_count) / probe_count : 0.0;
    printf("\n--- %s tcping statistics ---\n", ip);
    printf("%d probes transmitted, %d received, %.0f%% packet loss, time %.0fms\n",
        probe_count, recv_count, loss, total_time);
    if (recv_count > 0) {
        printf("rtt min/avg/max/mdev = %.3f/%.3f/%.3f/%.3f ms\n", min, avg, max, mdev);
    }
    printf("up: %.3fs, down: %.3fs\n", up_time, down_time);
    free(rtt_list);
    return 0;
}