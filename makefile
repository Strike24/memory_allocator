CC = gcc
CFLAGS = -Wall -Wextra -g -std=gnu11 -Iinclude -pthread

SRCS = main.c src/allocator.c src/chunks.c

allocator: $(SRCS) src/security.c
	$(CC) $(CFLAGS) -o allocator $(SRCS) src/security.c

notsecured: $(SRCS) src/no_security.c
	$(CC) $(CFLAGS) -o allocator_not_secured $(SRCS) src/no_security.c



clean:
	rm -f allocator