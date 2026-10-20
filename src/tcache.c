#include "internals.h"
#include "allocator.h"
#include <pthread.h>

#define TCACHE_MAX_BINS 7

typedef struct tcache_chunk
{
    struct tcache_chunk *next;
    struct tcache_thread *key;

} tcache_chunk;

typedef struct tcache_thread
{
    char counts[TCACHE_MAX_BINS];                   // number of chunks in each bin
    struct tcache_chunk *enteries[TCACHE_MAX_BINS]; // singly linked head pointers for bins
} tcache_thread;

__thread tcache_thread *per_thread_tcache = NULL;

static void tcache_init()
{
    size_t tcache_size = sizeof(tcache_thread);

    per_thread_tcache = (tcache_thread *)salloc_internal(tcache_size);
    memset(per_thread_tcache, 0, tcache_size);
}

void *tcache_get(size_t size)
{
    // Lazy initialize tcache if not initialized for specific thread yet
    if (per_thread_tcache == NULL)
    {
        // Lock heap mutex to prevent salloc from calling tcache_init
        pthread_mutex_lock(&heap_lock);
        tcache_init();
        pthread_mutex_unlock(&heap_lock);
        return NULL;
    }
}