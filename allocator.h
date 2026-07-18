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

// used to verify a chunk's integrety
#define MAGIC_NUM 0xDEADBEEF
#define OVERWRITE_HEX 0xDE
#define NUM_BINS 10
#define MIN_CHUNK_SIZE 16
#define HEADER_SIZE offsetof(heapchunk, payload)
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

typedef struct heapinfo
{
    heapchunk *bins[NUM_BINS];
    bool initalized;
    size_t avail;
} heapinfo;

// ==================
// PUBLIC Functions
// ==================

// Main allocator function, takes a size and returns a curated memory chunk
void *balloc(size_t size);
// Frees the memory chunk and adds it back to the freelist, while verifying chunk's integrity
void bfree(void *memory);
// Resizes the size of a memory block
void *brealloc(void *memory, size_t size);

// ==================
// Internal Functions
// ==================

// Initializes a new heap and asks OS for virtual continuous addresses
static int init_heap(heapinfo *heap);

// Searches for free chunks from the bins in O(1) at the average case
static heapchunk *find_free_chunk(size_t size);

// Calculates and classefies required bin based on chunk's size
static int get_bin_index(size_t size);

static void init_canary();
static inline size_t calculate_canary(heapchunk *chunk);

// Splits one big chunk to save the unnecessary memory
static void split_chunk(heapchunk *avail_chunk, size_t requested_size);

// Asks OS for more addresses when needing more space than first mapped
static heapchunk *increase_heap(size_t required_space);
static void *request_space(size_t required_space);

static void add_to_bin(heapchunk *chunk);
static void remove_from_bin(heapchunk *chunk);

// merge chunk next into chunk original
static void merge_adj_chunks(heapchunk *original, heapchunk *next);
static heapchunk *next_phyiscal_chunk(heapchunk *current);
static heapchunk *get_validated_chunk(void *memory);

static void advise_free(heapchunk *chunk);