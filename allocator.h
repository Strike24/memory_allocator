#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <stddef.h>

// used to verify a chunk's integrety
#define MAGIC_NUM 0xDEADBEEF
#define NUM_BINS 10

typedef struct heapchunk
{
    size_t size;
    bool is_inuse;
    uint32_t magic_num;

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

typedef struct
{
    heapchunk *bins[NUM_BINS];
    bool initalized;
    size_t avail;
} heapinfo;

// Main allocator function, takes a size and returns a curated memory chunk
void *balloc(size_t size);
// Frees the memory chunk and adds it back to the freelist, while verifying chunk's integrity
void bfree(void *memory);

// Initializes a new heap with 1 memory page
int init_heap(heapinfo *heap);

// Searches for free chunks from the bins in O(1) at the average case
void find_free_chunk(heapchunk **output_ptr, size_t size);

// Calculates and classefies required bin based on chunk's size
int get_bin_index(size_t size);

// Splits one big chunk to save the unnecessary memory
void split_chunk(heapchunk *avail_chunk, size_t requested_size);

// Asks the OS for more memory when the heap runs out
int increase_heap(heapchunk **output_ptr, size_t required_space);
void *request_space(size_t required_space);

void add_to_bin(heapchunk *chunk);
void remove_from_bin(heapchunk *chunk);