#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <stddef.h>

// Main allocator function, takes a size and returns a curated memory chunk
void *salloc(size_t size);
// Frees the memory chunk and adds it back to the freelist, while verifying chunk's integrity
void sfree(void *memory);
// Resizes the size of a memory block
void *srealloc(void *memory, size_t size);
void print_debug();

#endif