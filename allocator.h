#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>

typedef struct heapchunk_h
{
    size_t size;
    bool is_inuse;
    struct heapchunk_h *next_free;
} heapchunk_h;

typedef struct
{
    heapchunk_h *start;
    uint32_t avail;
} heapinfo;

int init_heap(heapinfo *heap);