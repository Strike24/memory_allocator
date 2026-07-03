#include "allocator.h"

static heapinfo heap = {0};
static size_t global_cookie = 0;
static pthread_mutex_t heap_lock = PTHREAD_MUTEX_INITIALIZER; // Mutex lock to allow thread-safe allocation

void *balloc(size_t size)
{
    // lazy init
    if (heap.initalized == 0)
    {
        int rc = init_heap(&heap);
        if (rc != 0)
            return NULL;
    }

    pthread_mutex_lock(&heap_lock);

    if (size < MIN_CHUNK_SIZE) // Min chunk size 16 so list pointers have a space when freed
        size = MIN_CHUNK_SIZE;

    // Align chunk size
    size = (size + 15) & ~15;

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

    if (free->size > size + HEADER_SIZE)
    {
        split_chunk(free, size);
    }

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

    heapchunk *chunk = (heapchunk *)((char *)memory - offsetof(heapchunk, payload)); // Get the original chunk header

    // Verify chunk integrety
    if (chunk->canary != calculate_canary(chunk))
    {
        fprintf(stderr, "chunk canary cookie got corrupted, aborting.\n");
        pthread_mutex_unlock(&heap_lock);
        abort();
        return;
    }

    if (chunk->is_inuse == false)
    {
        fprintf(stderr, "Double free detected, aborting to avoid corruption.\n");
        pthread_mutex_unlock(&heap_lock);
        abort();
        return;
    }

    chunk->is_inuse = false;

    heapchunk *next = next_phyiscal_chunk(chunk);
    if (next != NULL && next->is_inuse == false)
    {
        merge_adj_chunks(chunk, next);
    }

    // Add back to freelist
    add_to_bin(chunk);
    pthread_mutex_unlock(&heap_lock);

    return;
}

static void merge_adj_chunks(heapchunk *original, heapchunk *next)
{
    remove_from_bin(next);

    // Original now has next's size, and the size of next's header as its in use anymore
    size_t total = original->size + next->size + HEADER_SIZE;
    original->size = total;
}

static heapchunk *next_phyiscal_chunk(heapchunk *current)
{
    if (current == NULL)
        return NULL;

    size_t current_size = current->size;

    // Get next physical chunk in memory
    heapchunk *right_neighbor = (heapchunk *)((char *)current + HEADER_SIZE + current_size);
    if (right_neighbor->canary != calculate_canary(right_neighbor))
        return NULL;

    return right_neighbor;
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

static void add_to_bin(heapchunk *chunk)
{
    // connect new chunk to the freelist
    int bin_index = get_bin_index(chunk->size);
    heapchunk *bin_head = heap.bins[bin_index];
    if (bin_head != NULL)
    {
        bin_head->list.prev = chunk;
        chunk->list.prev = NULL;
        chunk->list.next = bin_head;
    }
    else
    {
        chunk->list.next = NULL;
        chunk->list.prev = NULL;
    }

    heap.bins[bin_index] = chunk;
    heap.avail += chunk->size;
}

static void remove_from_bin(heapchunk *chunk)
{
    int bin_index = get_bin_index(chunk->size);

    // Disconnecting the previous block
    if (chunk->list.prev != NULL)
    {

        // validate pointers for safe unlinking
        if (chunk->list.prev->list.next != chunk)
        {
            fprintf(stderr, "detected a corrupted doubly-linked list, aborting.\n");
            abort();
        }

        chunk->list.prev->list.next = chunk->list.next;
    }
    else
        heap.bins[bin_index] = chunk->list.next;

    // Disconnecting the next block
    if (chunk->list.next != NULL)
    {
        // validate pointers for safe unlinking
        if (chunk->list.next->list.prev != chunk)
        {
            fprintf(stderr, "detected a corrupted doubly-linked list, aborting.\n");
            abort();
        }

        chunk->list.next->list.prev = chunk->list.prev;
    }

    chunk->list.next = NULL;
    chunk->list.prev = NULL;
    chunk->is_inuse = true;

    heap.avail -= chunk->size;
}

static void split_chunk(heapchunk *avail_chunk, size_t requested_size)
{
    // shorten available chunk's size to the requested size
    size_t original_size = avail_chunk->size;
    avail_chunk->size = requested_size;

    // create a new chunk where the user allocated memory ends
    size_t remainder_size = original_size - requested_size - HEADER_SIZE;

    heapchunk *new_chunk = (heapchunk *)((char *)avail_chunk + HEADER_SIZE + requested_size);
    new_chunk->size = remainder_size;
    new_chunk->is_inuse = false;
    new_chunk->canary = calculate_canary(new_chunk);

    // connect new_chunk to the freelist
    add_to_bin(new_chunk);
}

static heapchunk *find_free_chunk(size_t size)
{
    int bin_number = get_bin_index(size);

    heapchunk *current = NULL;
    // Search for free chunks in size, if non available check in bigger bins
    for (int i = bin_number; i < NUM_BINS; i++)
    {
        current = heap.bins[i];
        while (current != NULL)
        {
            if (current->is_inuse == false && current->size >= size)
            {
                return current;
            }
            current = current->list.next;
        }
    }

    return NULL;
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

static int get_bin_index(size_t size)
{
    if (size <= 32) // Base case
    {
        return 0;
    }

    size_t s = size - 1;
    int shifts = 0;
    while (s > 0)
    {
        s >>= 1;
        shifts++;
    }

    int index = shifts - 5;
    if (index >= NUM_BINS)
        return NUM_BINS - 1; // If over max bin put in max bin

    return index;
}

static int init_heap(heapinfo *heap)
{
    init_canary();

    size_t page_size = sysconf(_SC_PAGESIZE);
    // Ask the system for a memory page
    void *mapped_memory = request_space(page_size);

    // Init headblock for the heap
    heapchunk *first = (heapchunk *)mapped_memory;
    first->is_inuse = false;
    first->size = page_size - HEADER_SIZE; // size left without the heapchunk header
    first->canary = calculate_canary(first);

    add_to_bin(first);
    heap->initalized = true;

    printf("MMAP mapped page starting at: %p\n", mapped_memory);

    return 0;
}

static void init_canary()
{
    FILE *urandom = fopen("/dev/urandom", "r");
    if (urandom != NULL)
    {
        fread(&global_cookie, sizeof(size_t), 1, urandom);
        fclose(urandom);
    }
    else
    {
        // Fallback when urandom doesnt work
        global_cookie = MAGIC_NUM ^ (size_t)&global_cookie;
    }
}

static inline size_t calculate_canary(heapchunk *chunk)
{
    return global_cookie ^ (size_t)chunk;
}

static void print_debug()
{
    printf("-=- Heap Debug Information -=-\n");
    printf("- Available space: %ld bytes\n", heap.avail);
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

int main()
{
    int *ptr = (int *)balloc(32);

    *ptr = 515;
    printf("Test: %p\n", ptr);
    printf("Test: %d\n", *ptr);
    bfree(ptr);
    print_debug();

    return 0;
}