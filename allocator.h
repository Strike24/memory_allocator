#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>

// used to verify a chunk's integrety
#define MAGIC_NUM 0xDEADBEEF

typedef struct heapchunk
{
    size_t size;
    bool is_inuse;
    uint32_t magic_num;
    struct heapchunk *next;
} heapchunk;

typedef struct
{
    heapchunk *start;
    uint32_t avail;
} heapinfo;

int init_heap(heapinfo *heap);

void *balloc(size_t size);
void bfree(void *memory);

void split_chunk(heapchunk *avail_chunk, size_t requested_size);
int increase_heap(heapchunk **output_ptr, size_t required_space);
void *request_space(size_t required_space);
void find_free_chunk(heapchunk **output_ptr, size_t size);