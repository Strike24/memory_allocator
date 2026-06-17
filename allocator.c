#include "allocator.h"

int main()
{
    heapinfo heap = {};
    init_heap(&heap);
    return 0;
}

int init_heap(heapinfo *heap)
{
    long page_size = sysconf(_SC_PAGESIZE);
    // Ask the system for a memory page
    void *mapped_memory = mmap(NULL, page_size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

    if (mapped_memory == MAP_FAILED)
    {
        perror("MMap failed to map memory.");
        return -1;
    }

    // Init headblock for the heap
    heapchunk_h *first = (heapchunk_h *)mapped_memory;
    first->is_inuse = false;
    first->size = page_size - sizeof(heapchunk_h); // size left without the heapchunk header
    first->next_free = NULL;

    heap->start = first;
    heap->avail = first->size;

    printf("MMAP mapped page starting at: %p", mapped_memory);

    return 0;
}
