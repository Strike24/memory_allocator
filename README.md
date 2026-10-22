# Salloc - Secure Allocator

A custom **secure** memory allocator in C with tcache, heap hardening, size-class bins, chunk splitting and bidirectional coalescing. Built to explore allocator internals and heap security.
Built with the knowledge I aquired from the heap module at [pwn.college](https://pwn.college/).

I wrote about building it here: [Building a Memory Allocator](https://strike24.github.io/posts/projects/building-a-memory-allocator/). The post is a bit older than the code, so trust the code.

## How it works

- **Per-Thread Cache (tcache):** Each thread maintains its own local cache of recently freed chunks, serving common allocations lock-free and bypassing global mutex contention on the fast path.
- Memory comes from 2MB `mmap` arenas, carved into chunks with a small header.
- Free chunks (that bypass \ overflow tcache) sit in 10 size-class bins (powers of two), doubly linked, first fit.
- Big chunks get split on alloc. Free chunks get merged with both neighbours on free.
- Big freed ranges (1 page and above) are handed back to the OS with `madvise(MADV_FREE)`.
- When a thread is done, its chunks are returned to the global arena, and the thread cache is cleared.

## Hardening

- **Canaries:** each chunk header has a 32-bit canary (random cookie XOR the chunk address). Checked on free, on the next neighbour, and while searching bins. A bad canary there aborts.
- **Safe linking**: tcache fd pointers are xor'd with their offseted address, utilizing the ASLR to prevent heap metadata corruption. Heavly inspired by [checkpoint's addition to ptmalloc](https://research.checkpoint.com/2020/safe-linking-eliminating-a-20-year-old-malloc-exploit-primitive/).
- **Safe unlinking:** `prev->next` and `next->prev` are checked before a chunk leaves a bin.
- **Double free:** freeing a chunk that's already free aborts.
- **Boundary chunk:** every arena ends with a zero-size chunk so walking to the next neighbour stops at the end of the arena.

## TCache

Tcache is added to improve efficency, and with simmilar structure to ptmalloc. Each thread maintains its own private array of singly-linked lists for small-to-medium size classes. When a chunk is freed, it is placed directly into the calling thread's local tcache bin rather than acquiring the global mutex and coalescing immediately. Subsequent allocations of the same size class are served straight from the tcache in $O(1)$ time with zero lock overhead, falling back to the central arena and global mutex only when a local bin is empty or full.

## Build

```
make libsalloc      # make shared library
make demo           # make demo program in main.c linked with the library
make notsecured     # make a non-secure version of the library+demo
```

The API is in `include/allocator.h`: `salloc`, `sfree`, `srealloc`, `print_debug`.

## Status

Work in progress. Next up:

- [v] safe linking
- [v] thread cache (right now it's one lock)
- [ ] tests and a fuzzer
- [ ] vulnerability "lab" - examples of exploits and how the hardening prevents them
