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

void abort_doublefree()
{
    fprintf(stderr, "Double free detected, aborting to avoid corruption.\n");
    abort();
}

void init_canary()
{
    FILE *urandom = fopen("/dev/urandom", "r");
    if ((urandom != NULL) && (fread(&global_cookie, sizeof(size_t), 1, urandom) == 1))
    {
        fclose(urandom);
    }
    else
    {
        perror("/dev/urandom failed to be read");
        abort();
    }
}

inline uint32_t calculate_canary(heapchunk *chunk)
{
    return (uint32_t)(global_cookie ^ (size_t)chunk);
}
