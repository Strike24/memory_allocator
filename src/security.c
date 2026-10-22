#include "internals.h"

heapchunk *get_validated_chunk(void *memory)
{
    if (memory == NULL)
        return NULL;

    heapchunk *chunk = (heapchunk *)((char *)memory - offsetof(heapchunk, payload));

    if (chunk->canary != calculate_canary(chunk))
    {
        abort_canary();
    }
    if (chunk->is_inuse == false)
    {
        abort_doublefree();
    }
    return chunk;
}

void abort_canary()
{
    fprintf(stderr, "chunk canary cookie got corrupted, aborting.\n");
    abort();
}

void abort_corruption()
{
    fprintf(stderr, "Heap corruption detected (salloc), aborting.\n");
    abort();
}

void abort_doublefree()
{
    fprintf(stderr, "Double free detected, aborting to avoid corruption.\n");
    abort();
}

void init_canary()
{
    if (getrandom(&global_cookie, sizeof(global_cookie), 0) != sizeof(global_cookie))
    {
        perror("getrandom() failed");
        abort();
    }
}

inline uint32_t calculate_canary(heapchunk *chunk)
{
    return (uint32_t)(global_cookie ^ (size_t)chunk);
}
