# Memory Allocator

Custom memory allocator (malloc) implemented in C.
balloc - bad allocator :)
Work in progress

## How it works

A **"Segregated Bins"** allocator that uses a free list to manage allocated memory chunks.
The bins are organized by size classes, each containing a list of free aligned memory chunks, assuring O(1) allocation complexity (At least in the average case)
The allocator uses a **best-fit** strategy to find the best chunk for allocation, and it splits larger chunks when necessary. When a chunk is freed, it is added back to the free list and is merged with adjacent free chunks if possible.

The allocator also includes a mutex lock to ensure thread safety during allocation and deallocation.

Heap Mitigations currently implemented:

- Safe Unlinking & Double Free:
  integrity checking of the free list pointers before unlinking to ensure they did not get altered, and checking if a chunk is already freed before adding it back to the free list, preventing double free vulnerabilities.
- Use-After-Free:
  overwrite freed chunks with garbage data to prevent use-after-free vulnerabilities.
- Heap Canaries ("Security Cookies"):
  canary value at the header of each chunk to detect buffer overflows. If the canary is altered, the allocator aborts the program.
