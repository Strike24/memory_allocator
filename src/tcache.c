#include "internals.h"
#include "allocator.h"
#include <pthread.h>

#define TCACHE_MAX_BINS 64
#define TCACHE_FILL_COUNT 7

typedef struct tcache_chunk
{
    struct tcache_chunk *next;
    struct tcache_thread *key;

} tcache_chunk;

typedef struct tcache_thread
{
    char counts[TCACHE_MAX_BINS];                  // number of chunks in each bin
    struct tcache_chunk *entries[TCACHE_MAX_BINS]; // singly linked head pointers for bins
} tcache_thread;

__thread tcache_thread *per_thread_tcache = NULL;

// for thread destructor
static pthread_key_t tcache_destructor_key;
static pthread_once_t tcache_key_once = PTHREAD_ONCE_INIT;

// forward decl
static void tcache_list_add(tcache_chunk *chunk, int bin);
static void tcache_list_remove(tcache_chunk *chunk, int bin);
static inline int get_tcache_idx(size_t size);

static void tcache_thread_destructor()
{
    if (per_thread_tcache != NULL)
    {
        tcache_flush();

        pthread_mutex_lock(&heap_lock);
        sfree_internal(per_thread_tcache);
        pthread_mutex_unlock(&heap_lock);

        per_thread_tcache = NULL;
    }
}

static void make_tcache_key(void)
{
    pthread_key_create(&tcache_destructor_key, tcache_thread_destructor);
}

static void tcache_init()
{
    pthread_once(&tcache_key_once, make_tcache_key);

    pthread_mutex_lock(&heap_lock);
    size_t tcache_size = sizeof(tcache_thread);

    per_thread_tcache = (tcache_thread *)salloc_internal(tcache_size);
    memset(per_thread_tcache, 0, tcache_size);
    pthread_mutex_unlock(&heap_lock);

    pthread_setspecific(tcache_destructor_key, per_thread_tcache);
}

void *tcache_get(size_t size)
{
    // Lazy initialize tcache if not initialized for specific thread yet
    if (per_thread_tcache == NULL)
    {
        tcache_init();
        return NULL;
    }

    // tcache already initialized, size is aligned
    int bin = get_tcache_idx(size);
    if (bin >= TCACHE_MAX_BINS)
        return NULL; // too big for tcache

    if (per_thread_tcache->counts[bin] < 1)
        return NULL; // empty bin

    tcache_chunk *chunk = per_thread_tcache->entries[bin];
    tcache_list_remove(chunk, bin);

    return chunk;
}

bool tcache_put(heapchunk *chunk)
{
    if (chunk == NULL)
        return false;

    // Lazy initialize tcache if not initialized for specific thread yet
    if (per_thread_tcache == NULL)
    {
        tcache_init();
    }

    int bin = get_tcache_idx(chunk->size);
    if (bin >= TCACHE_MAX_BINS)
        return false; // too big for tcache

    if (per_thread_tcache->counts[bin] >= TCACHE_FILL_COUNT)
        return false; // bin is full

    chunk->list.next = NULL;
    chunk->list.prev = NULL;
    tcache_list_add((tcache_chunk *)chunk->payload, bin);

    return true;
}

void tcache_flush(void)
{
    if (per_thread_tcache == NULL)
        return;

    for (int i = 0; i < TCACHE_MAX_BINS; i++)
    {
        for (int j = 0; j < per_thread_tcache->counts[i]; j++)
        {
            tcache_chunk *head = per_thread_tcache->entries[i];

            tcache_list_remove(head, i);
            sfree_internal(head);
        }
    }
}

static void tcache_list_add(tcache_chunk *chunk, int bin)
{
    if (chunk == NULL)
        return;

    tcache_chunk *old_head = per_thread_tcache->entries[bin];

    chunk->key = per_thread_tcache;
    chunk->next = old_head;

    per_thread_tcache->entries[bin] = chunk;
    per_thread_tcache->counts[bin]++;
}

static void tcache_list_remove(tcache_chunk *chunk, int bin)
{
    if (chunk == NULL)
        return;

    tcache_chunk *next = chunk->next;

    chunk->key = NULL; // disconnect chunk from list and prevent double freeing

    per_thread_tcache->entries[bin] = next; // make the next chunk head
    per_thread_tcache->counts[bin]--;
}

static inline int get_tcache_idx(size_t size)
{
    return (size - 1) / 16;
}