#include "internals.h"

heapchunk *get_validated_chunk(void *memory)
{
    if (memory == NULL)
        return NULL;

    heapchunk *chunk = (heapchunk *)((char *)memory - offsetof(heapchunk, payload));

    // disabled on "no_security" mode
    // if (chunk->canary != calculate_canary(chunk))
    // {
    //     abort_canary();
    // }
    // if (chunk->is_inuse == false)
    // {
    //     abort_doublefree();
    // }

    return chunk;
}

void init_canary()
{
    // FILE *urandom = fopen("/dev/urandom", "r");
    // if (urandom != NULL)
    // {
    //     fread(&global_cookie, sizeof(size_t), 1, urandom);
    //     fclose(urandom);
    // }
    // else
    // {
    //     // Fallback when urandom doesnt work
    //     global_cookie = MAGIC_NUM ^ (size_t)&global_cookie;
    // }
    return;
}

inline uint32_t calculate_canary(heapchunk *chunk)
{
    // canary disabled (0) on no security mode
    // return (uint32_t)(global_cookie ^ (size_t)chunk);
    return 0;
}
