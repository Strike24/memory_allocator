CC = gcc
CFLAGS = -Wall -Wextra -g -std=gnu11 -Iinclude -pthread

SRCS = main.c src/allocator.c src/chunks.c src/security.c

allocator: $(SRCS)
	$(CC) $(CFLAGS) -o allocator $(SRCS)

clean:
	rm -f allocator