#include "allocator.h"

static heapinfo heap = {0};

void *balloc(size_t size)
{
    // lazy init
    if (heap.start == NULL)
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

    if (free->size > size + sizeof(heapchunk))
    {
        split_chunk(free, size);
        heap.avail -= sizeof(heapchunk);
    }

    free->is_inuse = true;
    heap.avail -= size;
    // Skip over the header, return the memory chunk
    void *allocated_memory = (void *)(free + 1);
    return allocated_memory;
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

    // connect new chunk to the heap
    heapchunk *heap_end = heap.start;
    while (heap_end->next != NULL)
        heap_end = heap_end->next;
    heap_end->next = new_chunk;

    heap.avail += new_chunk->size;
    *output_ptr = new_chunk;
    return 0;
}

void bfree(void *memory)
{
    if (memory == NULL)
        return;

    heapchunk *chunk = (heapchunk *)memory - 1; // Get the original chunk header

    // Verify chunk integrety
    if (chunk->magic_num != MAGIC_NUM)
        return;

    chunk->is_inuse = false;
    heap.avail += chunk->size;
    //! In the future, merge free chunks to avoid fragmentation (coalescing)
    return;
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

    // connect new_chunk to the heap list right after avail_chunk
    heapchunk *temp = avail_chunk->next;
    avail_chunk->next = new_chunk;
    new_chunk->next = temp;
}

void find_free_chunk(heapchunk **output_ptr, size_t size)
{
    heapchunk *current = heap.start;
    while (current != NULL)
    {
        if (current->is_inuse == false && current->size >= size)
        {
            *output_ptr = current;
            return;
        }
        current = current->next;
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
    first->next = NULL;
    first->magic_num = MAGIC_NUM;

    heap->start = first;
    heap->avail = first->size;

    printf("MMAP mapped page starting at: %p\n", mapped_memory);

    return 0;
}

int main()
{
    int *ptr = (int *)balloc(32);
    int *ptr3 = (int *)balloc(5000);

    printf("Test: %p\n", ptr3);

    bfree(ptr);
    bfree(ptr3);

    return 0;
}