#include "internals.h"

heapchunk *get_validated_chunk(void *memory)
{
    if (memory == NULL)
        return NULL;

    heapchunk *chunk = (heapchunk *)((char *)memory - offsetof(heapchunk, payload));

    if (chunk->canary != calculate_canary(chunk))
    {
        fprintf(stderr, "chunk canary cookie got corrupted, aborting.\n");
        abort();
    }
    if (chunk->is_inuse == false)
    {
        fprintf(stderr, "Double free detected, aborting to avoid corruption.\n");
        abort();
    }
    return chunk;
}

void init_canary()
{
    FILE *urandom = fopen("/dev/urandom", "r");
    if (urandom != NULL)
    {
        fread(&global_cookie, sizeof(uint32_t), 1, urandom);
        fclose(urandom);
    }
    else
    {
        // Fallback when urandom doesnt work
        global_cookie = (uint32_t)(MAGIC_NUM ^ (size_t)&global_cookie);
    }
}

inline uint32_t calculate_canary(heapchunk *chunk)
{
    return (uint32_t)(global_cookie ^ (size_t)chunk);
}
