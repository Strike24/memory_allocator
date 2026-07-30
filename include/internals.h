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
#define ROUND_DOWN_PAGE(n, page_size) ((n) & ~((page_size) - 1))
#define ROUND_UP_PAGE(n, page_size) (((n) + (page_size) - 1) & ~((page_size) - 1))
#define ALIGNMENT 16
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1))
#define ARENA_SIZE (2 * 1024 * 1024) // 2MB
#define PAGE_SIZE sysconf(_SC_PAGESIZE)
#define REQ_PAGES_TO_FREE 1

typedef struct heapchunk
{
    size_t size;
    bool is_inuse;
    size_t canary;

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

#define HEADER_SIZE offsetof(heapchunk, payload)

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

#endif // INTERNALS_H