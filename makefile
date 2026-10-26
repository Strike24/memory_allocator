CC = gcc
CFLAGS = -Wall -Wextra -g -std=gnu11 -Iinclude -pthread

SRCS = src/allocator.c src/chunks.c src/tcache.c

# shared library
libsalloc.so: $(SRCS) src/security.c
	$(CC) $(CFLAGS) -fPIC -shared -o $@ $(SRCS) src/security.c

unsecured: CFLAGS += -DUNSECURED
unsecured: libsalloc.so

# demo linked against the library
demo: main.c libsalloc.so
	$(CC) $(CFLAGS) -o $@ main.c -L. -lsalloc -Wl,-rpath,'$$ORIGIN'


.PHONY: clean unsecured
clean:
	rm -f allocator demo libsalloc.so
