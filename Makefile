CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c99 -O2
LDFLAGS = -lm
SRCS = src/tcp_probe.c src/common.c
OBJS = $(SRCS:.c=.o)
TARGET = tcp_probe

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean