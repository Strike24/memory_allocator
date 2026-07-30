#include "allocator.h"
#include "internals.h"

#define _GNU_SOURCE
// allocator.c internals
static int init_heap(heapinfo *heap);
static void *request_space(size_t required_space);
static heapchunk *increase_heap(size_t required_space);
static void advise_free(heapchunk *chunk);

heapinfo heap = {0};
size_t global_cookie = 0;
static pthread_mutex_t heap_lock = PTHREAD_MUTEX_INITIALIZER; // Mutex lock to allow thread-safe allocation

void *balloc(size_t size)
{
    pthread_mutex_lock(&heap_lock);

    // lazy init
    if (heap.initalized == 0)
    {
        int rc = init_heap(&heap);
        if (rc != 0)
            return NULL;
    }

    if (size < MIN_CHUNK_SIZE) // Min chunk size 16 so list pointers have a space when freed
        size = MIN_CHUNK_SIZE;

    // Align chunk size
    size = ALIGN(size);

    // loop over free memory chunks to see if size is available
    heapchunk *free = find_free_chunk(size);

    if (free == NULL)
    {
        free = increase_heap(size);
        if (free == NULL)
        {
            perror("No available memory left or error occured.\n");
            pthread_mutex_unlock(&heap_lock);
            return NULL;
        }
    }

    // Remove allocated chunk from the freelist
    remove_from_bin(free);

    // attempt to split chunk if enough free space
    split_chunk(free, size);

    // Skip over the header, return the memory chunk
    void *allocated_memory = (void *)(free->payload);

    pthread_mutex_unlock(&heap_lock);
    return allocated_memory;
}

void bfree(void *memory)
{
    if (memory == NULL)
        return;

    pthread_mutex_lock(&heap_lock);

    heapchunk *chunk = get_validated_chunk(memory);

    chunk->is_inuse = false;

    // overwrite old data to prevent uaf's
    memset(chunk->payload, OVERWRITE_HEX, chunk->size);

    // merge next phyiscal chunk (if free and exists) to avoid fragmentation
    heapchunk *next = next_phyiscal_chunk(chunk);
    if (next != NULL && next->is_inuse == false)
    {
        merge_adj_chunks(chunk, next);
    }

    // Add back to freelist
    add_to_bin(chunk);
    advise_free(chunk);

    pthread_mutex_unlock(&heap_lock);
    return;
}

void *brealloc(void *memory, size_t size)
{
    pthread_mutex_lock(&heap_lock);
    if (memory == NULL)
    {
        pthread_mutex_unlock(&heap_lock);
        return balloc(size);
    }

    if (size < MIN_CHUNK_SIZE) // Min chunk size 16 so list pointers have a space when freed
        size = MIN_CHUNK_SIZE;

    // Align chunk size
    size = ALIGN(size);

    heapchunk *original_chunk = get_validated_chunk(memory);

    size_t current_size = original_chunk->size;

    // -- shrinking / not changing
    if (size <= current_size)
    {
        split_chunk(original_chunk, size);

        pthread_mutex_unlock(&heap_lock);
        return (void *)original_chunk->payload;
    }

    // -- growing
    heapchunk *next = next_phyiscal_chunk(original_chunk);

    // in place?
    if (next != NULL && next->is_inuse == false &&
        (current_size + HEADER_SIZE + next->size >= size))
    {
        merge_adj_chunks(original_chunk, next);

        // attempt to split if enough space
        split_chunk(original_chunk, size);

        pthread_mutex_unlock(&heap_lock);
        return (void *)original_chunk->payload;
    }

    // not enough space, must relocate
    pthread_mutex_unlock(&heap_lock);

    void *new_allocated = balloc(size);
    if (new_allocated == NULL)
    {
        return NULL;
    }

    // copy old chunk's data to the new allocated space
    memcpy(new_allocated, memory, current_size);

    // free old chunk
    bfree(original_chunk->payload);
    return new_allocated;
}
static heapchunk *increase_heap(size_t required_space)
{
    size_t page_size = sysconf(_SC_PAGESIZE);
    size_t total_required = required_space + HEADER_SIZE;

    size_t num_of_pages = (total_required + page_size - 1) / page_size;
    size_t bytes_to_request = num_of_pages * page_size;

    heapchunk *new_chunk = (heapchunk *)request_space(bytes_to_request);
    if (new_chunk == NULL)
        return NULL;

    new_chunk->is_inuse = false;
    new_chunk->canary = calculate_canary(new_chunk);

    new_chunk->size = bytes_to_request - HEADER_SIZE;

    // connect new chunk to the freelist
    add_to_bin(new_chunk);
    return new_chunk;
}

static void *request_space(size_t required_space)
{
    void *mapped_memory = mmap(NULL, required_space, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (mapped_memory == MAP_FAILED)
    {
        perror("MMap failed to map memory.\n");
        return NULL;
    }

    return mapped_memory;
}

static int init_heap(heapinfo *heap)
{
    init_canary();

    // Map virtual addresses to later be used when needed
    size_t arena_size = ARENA_SIZE;
    void *mapped_memory = request_space(arena_size);

    // Init headblock for the heap
    heapchunk *first = (heapchunk *)mapped_memory;
    first->is_inuse = false;
    first->size = arena_size - HEADER_SIZE; // size left without the heapchunk header
    first->canary = calculate_canary(first);

    add_to_bin(first);
    heap->initalized = true;

    printf("MMAP mapped page starting at: %p\n", mapped_memory);

    return 0;
}

static void advise_free(heapchunk *chunk)
{
    uintptr_t payload_start = (uintptr_t)chunk->payload + MIN_CHUNK_SIZE;
    uintptr_t payload_end = (uintptr_t)chunk->payload + chunk->size;

    if (payload_end == 0 || payload_start == 0)
        return;

    uintptr_t pages_start_address = ROUND_UP_PAGE(payload_start, PAGE_SIZE);
    uintptr_t pages_end_address = ROUND_DOWN_PAGE(payload_end, PAGE_SIZE);

    int pages_length = pages_end_address - pages_start_address;
    // if one or more pages fit in the chunk's payload,
    if ((pages_start_address < pages_end_address) && (pages_length >= REQ_PAGES_TO_FREE * PAGE_SIZE))
    {
        if (madvise((void *)pages_start_address, pages_length, MADV_FREE) == -1)
            perror("madvise failed");
    }
}

void print_debug()
{
    printf("-=- Heap Debug Information -=-\n");
    printf("- Avail space: %ld bytes\n", heap.avail);
    printf("- Number of bins in each bucket:\n");
    for (int i = 0; i < NUM_BINS; i++)
    {
        heapchunk *current = heap.bins[i];
        int count = 0;
        while (current != NULL)
        {
            count++;
            current = current->list.next;
        }
        printf("  * Bucket #%d - %d\n", i, count);
    }
}
