#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>

typedef struct heapchunk
{
    size_t size;
    bool is_inuse;
    struct heapchunk *next;
} heapchunk;

typedef struct
{
    heapchunk *start;
    uint32_t avail;
} heapinfo;

int init_heap(heapinfo *heap);

void *balloc(size_t size);

void split_chunk(heapchunk *avail_chunk, size_t requested_size);

bool find_free_chunk(heapchunk **output_ptr, size_t size);