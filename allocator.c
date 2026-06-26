#include "allocator.h"

static heapinfo heap = {0};

void *balloc(size_t size)
{
    // lazy init
    if (heap.initalized == 0)
    {
        int rc = init_heap(&heap);
        if (rc != 0)
            return NULL;
    }

    // loop over free memory chunks to see if size is available
    heapchunk *free = NULL;
    find_free_chunk(&free, size);

    if (free == NULL)
    {
        int rc = increase_heap(&free, size);
        if (rc < 0 || free == NULL)
        {
            perror("No available memory left or error occured.\n");
            return NULL;
        }
    }

    // Remove allocated chunk from the freelist
    remove_from_bin(free);

    if (free->size > size + sizeof(heapchunk))
    {
        split_chunk(free, size);
    }

    // Skip over the header, return the memory chunk
    void *allocated_memory = (void *)(free->payload);
    return allocated_memory;
}

void bfree(void *memory)
{
    if (memory == NULL)
        return;

    heapchunk *chunk = (heapchunk *)((char *)memory - offsetof(heapchunk, payload)); // Get the original chunk header

    // Verify chunk integrety
    if (chunk->magic_num != MAGIC_NUM)
        return;

    chunk->is_inuse = false;

    // Add back to freelist
    add_to_bin(chunk);

    //! In the future, merge free chunks to avoid fragmentation (coalescing)
    return;
}

int increase_heap(heapchunk **output_ptr, size_t required_space)
{
    size_t page_size = sysconf(_SC_PAGESIZE);
    size_t total_required = required_space + sizeof(heapchunk);

    size_t num_of_pages = (total_required + page_size - 1) / page_size;
    size_t bytes_to_request = num_of_pages * page_size;

    heapchunk *new_chunk = (heapchunk *)request_space(bytes_to_request);
    if (new_chunk == NULL)
        return -1;

    new_chunk->is_inuse = false;
    new_chunk->magic_num = MAGIC_NUM;

    new_chunk->size = bytes_to_request - sizeof(heapchunk);

    // connect new chunk to the freelist
    add_to_bin(new_chunk);
    *output_ptr = new_chunk;
    return 0;
}

void add_to_bin(heapchunk *chunk)
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

void remove_from_bin(heapchunk *chunk)
{
    int bin_index = get_bin_index(chunk->size);

    // Disconnecting the previous block
    if (chunk->list.prev != NULL)
        chunk->list.prev->list.next = chunk->list.next;
    else
        heap.bins[bin_index] = chunk->list.next;

    // Disconnecting the next block
    if (chunk->list.next != NULL)
        chunk->list.next->list.prev = chunk->list.prev;

    chunk->list.next = NULL;
    chunk->list.prev = NULL;
    chunk->is_inuse = true;

    heap.avail -= chunk->size;
}

void split_chunk(heapchunk *avail_chunk, size_t requested_size)
{
    // shorten available chunk's size to the requested size
    size_t original_size = avail_chunk->size;
    avail_chunk->size = requested_size;

    // create a new chunk where the user allocated memory ends
    size_t remainder_size = original_size - requested_size - sizeof(heapchunk);

    heapchunk *new_chunk = (heapchunk *)((char *)(avail_chunk + 1) + requested_size);
    new_chunk->size = remainder_size;
    new_chunk->is_inuse = false;
    new_chunk->magic_num = MAGIC_NUM;

    // connect new_chunk to the freelist
    add_to_bin(new_chunk);
}

void find_free_chunk(heapchunk **output_ptr, size_t size)
{
    // Get wanted bin number
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
                *output_ptr = current;
                return;
            }
            current = current->list.next;
        }
    }
}

void *request_space(size_t required_space)
{
    void *mapped_memory = mmap(NULL, required_space, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (mapped_memory == MAP_FAILED)
    {
        perror("MMap failed to map memory.\n");
        return NULL;
    }

    return mapped_memory;
}

int init_heap(heapinfo *heap)
{
    size_t page_size = sysconf(_SC_PAGESIZE);
    // Ask the system for a memory page
    void *mapped_memory = request_space(page_size);

    // Init headblock for the heap
    heapchunk *first = (heapchunk *)mapped_memory;
    first->is_inuse = false;
    first->size = page_size - sizeof(heapchunk); // size left without the heapchunk header
    first->magic_num = MAGIC_NUM;

    add_to_bin(first);
    heap->initalized = true;

    printf("MMAP mapped page starting at: %p\n", mapped_memory);

    return 0;
}

int get_bin_index(size_t size)
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

int main()
{
    int *ptr = (int *)balloc(32);

    *ptr = 515;
    printf("Test: %p\n", ptr);
    printf("Test: %d\n", *ptr);

    bfree(ptr);

    return 0;
}