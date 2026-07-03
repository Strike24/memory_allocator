# Memory Allocator

Custom memory allocator (malloc) implemented in C.
balloc - bad allocator :)
Work in progress

## How it works

A **binning** allocator that uses a free list to manage allocated memory chunks.
The bins are organized by size classes, each containing a list of free aligned memory chunks, assuring O(1) allocation complexity (At least in the average case).

The free list are doubly linked, designed to save memory by using a union to store the previous and next pointers only when the chunk is free. When the chunk is allocated, the data overrides them.

The allocator uses a **best-fit** strategy to find the best chunk for allocation, and it splits larger chunks when necessary. When a chunk is freed, it is added back to the free list and coalesced with adjacent free chunk if possible.

Heap Mitigations currently implemented:

- Safe Unlinking:
  The allocator checks the integrity of the free list pointers before unlinking to ensure they did not get altered
- Double Free:
  The allocator checks if a chunk is already freed before adding it back to the free list, and aborts if corrupt, preventing double free vulnerabilities.
- Heap Canaries ("Security Cookies"):
  The allocator adds a canary value at the header of each chunk to detect buffer overflows. If the canary is altered, the allocator aborts the program.
