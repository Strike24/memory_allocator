CC = gcc
CFLAGS = -Wall -Wextra -g -std=c99 -Iinclude

allocator: allocator.c
	${CC} ${CFLAGS} -o allocator allocator.c