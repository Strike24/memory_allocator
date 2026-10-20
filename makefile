CC = gcc
CFLAGS = -Wall -Wextra -g -std=gnu11 -Iinclude -pthread -fsanitize=address

SRCS = src/allocator.c src/chunks.c

# shared library
libsalloc.so: $(SRCS) src/security.c
	$(CC) $(CFLAGS) -fPIC -shared -o $@ $(SRCS) src/security.c

# demo linked against the library
demo: main.c libsalloc.so
	$(CC) $(CFLAGS) -o $@ main.c -L. -lsalloc -Wl,-rpath,'$$ORIGIN'



# Static builds (original targets)
notsecured: main.c $(SRCS) src/no_security.c
	$(CC) $(CFLAGS) -o allocator_not_secured $^


.PHONY: clean
clean:
	rm -f allocator allocator_not_secured demo libsalloc.so
