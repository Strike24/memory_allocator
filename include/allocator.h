#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <stddef.h>
#include <pthread.h>
#include <string.h>

// Main allocator function, takes a size and returns a curated memory chunk
void *salloc(size_t size);
// Frees the memory chunk and adds it back to the freelist, while verifying chunk's integrity
void sfree(void *memory);
// Resizes the size of a memory block
void *srealloc(void *memory, size_t size);
void print_debug();
