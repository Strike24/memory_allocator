CC = gcc
CFLAGS = -Wall -Wextra -g -std=gnu11 -Iinclude

allocator: allocator.c
	${CC} ${CFLAGS} -o allocator allocator.c