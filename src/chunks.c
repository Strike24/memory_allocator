#include "internals.h"

void split_chunk(heapchunk *avail_chunk, size_t requested_size)
{
    if (avail_chunk->size < requested_size + HEADER_SIZE + MIN_CHUNK_SIZE)
    {
        return; // not enough space to split
    }

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

heapchunk *find_free_chunk(size_t size)
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

void merge_adj_chunks(heapchunk *original, heapchunk *next)
{
    remove_from_bin(next);

    // original now has next's size, and the size of next's header as its in use anymore
    size_t total = original->size + next->size + HEADER_SIZE;
    original->size = total;
}

heapchunk *next_phyiscal_chunk(heapchunk *current)
{
    if (current == NULL)
        return NULL;

    size_t current_size = current->size;

    // get next physical chunk in memory
    heapchunk *right_neighbor = (heapchunk *)((char *)current + HEADER_SIZE + current_size);
    if (right_neighbor == NULL || right_neighbor->canary != calculate_canary(right_neighbor))
        return NULL;

    return right_neighbor;
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