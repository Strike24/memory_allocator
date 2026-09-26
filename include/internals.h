#ifndef INTERNALS_H
#define INTERNALS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <pthread.h>

// used to verify a chunk's integrety
#define MAGIC_NUM 0xDEADBEEF
#define OVERWRITE_HEX 0xDE
#define NUM_BINS 10
#define MIN_CHUNK_SIZE 16
#define ALIGNMENT 16
#define ARENA_SIZE (2 * 1024 * 1024) // 2MB
#define PAGE_SIZE sysconf(_SC_PAGESIZE)
#define REQ_PAGES_TO_FREE 1

typedef struct heapchunk
{
    size_t canary;
    size_t size;
    bool is_inuse;

    // next,prev pointers aren't needed when chunk is being used
    // Therfore, they can be replaced with the data when allocated
    union
    {
        struct
        {
            struct heapchunk *next;
            struct heapchunk *prev;
        } list;

        uint8_t payload[0]; // data label
    };
} heapchunk;

#define HEADER_SIZE 24

typedef struct heapinfo
{
    heapchunk *bins[NUM_BINS];
    bool initalized;
    size_t avail;
} heapinfo;

// shared globals
extern heapinfo heap;
extern size_t global_cookie;

// chunks.c
void add_to_bin(heapchunk *chunk);
void remove_from_bin(heapchunk *chunk);
void split_chunk(heapchunk *avail_chunk, size_t requested_size);
void merge_adj_chunks(heapchunk *original, heapchunk *next);
heapchunk *next_phyiscal_chunk(heapchunk *current);
heapchunk *find_free_chunk(size_t size);
int get_bin_index(size_t size);

// security.c
void init_canary(void);
size_t calculate_canary(heapchunk *chunk);
heapchunk *get_validated_chunk(void *memory);

uintptr_t round_down_page(uintptr_t n, size_t page_size)
{
    size_t remainder = n % page_size;
    return n - remainder;
}

uintptr_t round_up_page(uintptr_t n, size_t page_size)
{
    size_t remainder = n % page_size;

    if (remainder == 0)
        return n;

    return (n - remainder) + page_size;
}

size_t align_size(size_t size, size_t alignment)
{
    size_t remainder = size % alignment;

    if (remainder == 0)
        return size;

    return (size - remainder) + alignment;
}

#endif // INTERNALS_H