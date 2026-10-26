CC = gcc
CFLAGS = -Wall -Wextra -g -std=gnu11 -I../include -pthread

double_free_tcache: double_free_tcache.c ../libsalloc.so
	$(CC) $(CFLAGS) -o $@ double_free_tcache.c -L.. -lsalloc -Wl,-rpath,'$$ORIGIN/..'

tcache_poison: tcache_poison.c ../libsalloc.so
	$(CC) $(CFLAGS) -o $@ tcache_poison.c -L.. -lsalloc -Wl,-rpath,'$$ORIGIN/..'
