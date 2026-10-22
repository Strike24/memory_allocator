#include "internals.h"
#include "allocator.h"
#include <pthread.h>

#define TCACHE_MAX_BINS 64
#define TCACHE_FILL_COUNT 7
#define PAGE_SHIFT 12

// Safe-linking for fd pointer in tcache to prevent metadata corruption. (https://research.checkpoint.com/2020/safe-linking-eliminating-a-20-year-old-malloc-exploit-primitive/)
#define PROTECT_PTR(pos, ptr, type) \
    ((type)((((size_t)pos) >> PAGE_SHIFT) ^ ((size_t)ptr)))

#define REVEAL_PTR(pos, ptr, type) \
    PROTECT_PTR(pos, ptr, type)

#define aligned_OK(m) (((unsigned long)(m) & 15) == 0)

typedef struct tcache_chunk
{
    struct tcache_chunk *next; // protected with safe linking
    struct tcache_thread *key;

} tcache_chunk;

typedef struct tcache_thread
{
    char counts[TCACHE_MAX_BINS];                  // number of chunks in each bin
    struct tcache_chunk *entries[TCACHE_MAX_BINS]; // singly linked head pointers for bins
} tcache_thread;

static __thread tcache_thread *per_thread_tcache = NULL;

// for thread destructor
static pthread_key_t tcache_destructor_key;
static pthread_once_t tcache_key_once = PTHREAD_ONCE_INIT;

// forward decl
static void tcache_list_add(tcache_chunk *chunk, int bin);
static void tcache_list_remove(tcache_chunk *chunk, int bin);
static inline int get_tcache_idx(size_t size);

static void tcache_thread_destructor(void *data)
{
    (void)data;

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
    if (per_thread_tcache == NULL)
        return;

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

    tcache_chunk *tc = (tcache_chunk *)chunk->payload;
    if (tc->key == per_thread_tcache)
        abort_doublefree(); // if chunk already in tcache we double freed

    tcache_list_add(tc, bin);

    return true;
}

void tcache_flush(void)
{
    if (per_thread_tcache == NULL)
        return;

    for (int i = 0; i < TCACHE_MAX_BINS; i++)
    {

        while (per_thread_tcache->entries[i] != NULL)
        {
            tcache_chunk *head = per_thread_tcache->entries[i];

            if (!aligned_OK(head))
                abort_corruption();

            tcache_list_remove(head, i);

            pthread_mutex_lock(&heap_lock);
            sfree_internal(head);
            pthread_mutex_unlock(&heap_lock);
        }
    }
}

static void tcache_list_add(tcache_chunk *chunk, int bin)
{
    if (chunk == NULL)
        return;

    tcache_chunk *old_head = per_thread_tcache->entries[bin];

    chunk->key = per_thread_tcache;
    chunk->next = PROTECT_PTR(&chunk->next, old_head, tcache_chunk *);

    per_thread_tcache->entries[bin] = chunk;
    per_thread_tcache->counts[bin]++;
}

static void tcache_list_remove(tcache_chunk *chunk, int bin)
{
    if (chunk == NULL)
        return;

    tcache_chunk *next = REVEAL_PTR(&chunk->next, chunk->next, tcache_chunk *);

    chunk->key = NULL; // disconnect chunk from list and prevent double freeing

    per_thread_tcache->entries[bin] = next; // make the next chunk head
    per_thread_tcache->counts[bin]--;
}

static inline int get_tcache_idx(size_t size)
{
    return (size - 1) / 16;
}

void tcache_print(void)
{
    if (per_thread_tcache == NULL)
    {
        printf("[tcache] Uninitialized for this thread.\n");
        return;
    }

    printf("========================================\n");
    printf("   TCACHE STATE (Thread: %p)            \n", (void *)pthread_self());
    printf("========================================\n");

    int active_bins = 0;

    for (int i = 0; i < TCACHE_MAX_BINS; i++)
    {
        int count = per_thread_tcache->counts[i];

        // Skip empty bins to keep the output readable
        if (count == 0)
            continue;

        active_bins++;
        // Calculate the base size range for this bin index
        size_t bin_size = (i + 1) * 16;
        printf("Bin [%2d] (Size Class: %zu) | Count: %d/%d | Chunks: ",
               i, bin_size, count, TCACHE_FILL_COUNT);

        // Traverse the singly linked list of chunks in this bin
        tcache_chunk *current = per_thread_tcache->entries[i];
        while (current != NULL)
        {
            printf("[%p]", (void *)current);
            current = REVEAL_PTR(&current->next, current->next, tcache_chunk *);

            if (current != NULL)
                printf(" -> ");
        }
        printf("\n");
    }

    if (active_bins == 0)
    {
        printf("[tcache] All bins are currently empty.\n");
    }
    printf("========================================\n");
}
