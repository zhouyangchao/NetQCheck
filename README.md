# NetQCheck

A collection of simple, high-precision network quality testing tools written in C. Each tool is designed for reliability, extensibility, and zero compiler warnings.

## Build

```sh
make
```

## Tools

### tcp_probe
A TCP connectivity probe tool with configurable frequency, duration, and detailed logging.

**Usage:**
```
./tcp_probe [options] <host> [port]
```

**Arguments:**
- `<host>`            Target host (IP or domain) **[required]**
- `[port]`            Target port (default: 443)

**Options:**
- `-f`, `--freq`      Probe frequency in Hz (default: 1.0)
- `-d`, `--duration`  Probe duration in seconds (default: unlimited)
- `-q`, `--quiet`     Only print summary statistics
- `-h`, `--help`      Show help message

**Example:**
```
./tcp_probe -f 10 -d 3 1.1.1.1 80
```

**Sample output:**
```
TCPING 1.1.1.1:80 with 10.00 probes per second
tcp_seq=1 time=21.123 ms Connected
tcp_seq=2 time=20.987 ms Connected
...
--- 1.1.1.1 tcping statistics ---
30 probes transmitted, 30 received, 0% packet loss, time 3000ms
rtt min/avg/max/mdev = 20.987/21.123/21.456/0.123 ms
up: 3.000s, down: 0.000s
```

If `[port]` is omitted, the default is 443.
