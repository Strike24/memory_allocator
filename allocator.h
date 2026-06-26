#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>

// used to verify a chunk's integrety
#define MAGIC_NUM 0xDEADBEEF
#define NUM_BINS 10

typedef struct heapchunk
{
    size_t size;
    bool is_inuse;
    uint32_t magic_num;

    struct heapchunk *next;
    struct heapchunk *prev;

} heapchunk;

typedef struct
{
    heapchunk *bins[NUM_BINS];
    bool initalized;
    size_t avail;
} heapinfo;

int init_heap(heapinfo *heap);

void *balloc(size_t size);
void bfree(void *memory);
int get_bin_index(size_t size);
void split_chunk(heapchunk *avail_chunk, size_t requested_size);
int increase_heap(heapchunk **output_ptr, size_t required_space);
void *request_space(size_t required_space);
void find_free_chunk(heapchunk **output_ptr, size_t size);
void add_to_bin(heapchunk *chunk);
void remove_from_bin(heapchunk *chunk);