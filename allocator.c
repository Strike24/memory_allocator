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
    bool is_avail = find_free_chunk(&free, size);
    if (!is_avail || free == NULL)
    {
        // request_space() for more memory pages..
        perror("No available memory left\n");
        return NULL;
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

bool find_free_chunk(heapchunk **output_ptr, size_t size)
{
    heapchunk *current = heap.start;
    while (current != NULL)
    {
        if (current->is_inuse == false && current->size >= size)
        {
            *output_ptr = current;
            return true;
        }
        current = current->next;
    }
    return false;
}

int init_heap(heapinfo *heap)
{
    long page_size = sysconf(_SC_PAGESIZE);
    // Ask the system for a memory page
    void *mapped_memory = mmap(NULL, page_size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

    if (mapped_memory == MAP_FAILED)
    {
        perror("MMap failed to map memory.\n");
        return -1;
    }

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
    int *ptr2 = (int *)balloc(4016);

    bfree(ptr2);
    int *ptr3 = (int *)balloc(32);

    printf("Test: %p\n", ptr3);
    return 0;
}