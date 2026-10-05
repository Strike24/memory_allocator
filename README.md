# Salloc - Secure Allocator

A custom **secure** memory allocator in C with heap hardening, size-class bins, chunk splitting and bidirectional coalescing. Built to explore allocator internals and heap security.
Built with the knowledge I aquired from the heap module at [pwn.college](https://pwn.college/).

I wrote about building it here: [Building a Memory Allocator](https://strike24.github.io/posts/projects/building-a-memory-allocator/). The post is a bit older than the code, so trust the code.

## How it works

- Memory comes from 2MB `mmap` arenas, carved into chunks with a small header.
- Free chunks sit in 10 size-class bins (powers of two), doubly linked, first fit.
- Big chunks get split on alloc. Free chunks get merged with both neighbours on free.
- Big freed ranges are handed back to the OS with `madvise(MADV_FREE)`.
- One global mutex around alloc and free. Simple and coarse-grained.

## Hardening

- **Canaries:** each chunk header has a 32-bit canary (random cookie XOR the chunk address). Checked on free, on the next neighbour, and while searching bins. A bad canary there aborts.
- **Safe unlinking:** `prev->next` and `next->prev` are checked before a chunk leaves a bin.
- **Double free:** freeing a chunk that's already free aborts.
- **Boundary chunk:** every arena ends with a zero-size chunk so walking to the next neighbour stops at the end of the arena.
- **Safe linking**: Soon.

## Build

```
make allocator      # with hardening
make notsecured     # same allocator, checks off (for comparing exploits)
```

The API is in `include/allocator.h`: `salloc`, `sfree`, `srealloc`, `print_debug`.

## Status

Work in progress. Next up:

- [ ] safe linking
- [ ] thread cache (right now it's one lock)
- [ ] tests and a fuzzer
- [ ] vulnerability "lab" - examples of exploits and how the hardening prevents them
