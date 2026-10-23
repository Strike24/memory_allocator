// When UNSECURED is defined mitigations are turned off in the entire allocator for testing and demonstration purposes.

#include "internals.h"

heapchunk *get_validated_chunk(void *memory)
{
    if (memory == NULL)
        return NULL;

    heapchunk *chunk = (heapchunk *)((char *)memory - offsetof(heapchunk, payload));

#ifndef UNSECURED
    if (chunk->canary != calculate_canary(chunk))
    {
        abort_canary();
    }
    if (chunk->is_inuse == false)
    {
        abort_doublefree();
    }
#endif

    return chunk;
}
#ifndef UNSECURED
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
#else
void abort_canary() {}
void abort_corruption() {}
void abort_doublefree() {}
#endif

void init_canary()
{
#ifndef UNSECURED
    if (getrandom(&global_cookie, sizeof(global_cookie), 0) != sizeof(global_cookie))
    {
        perror("getrandom() failed");
        abort();
    }
#endif
}

inline uint32_t calculate_canary(heapchunk *chunk)
{
#ifndef UNSECURED
    return (uint32_t)(global_cookie ^ (size_t)chunk);
#else
    (void)chunk;
    return 0;
#endif
}
