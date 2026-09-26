# Secure Memory Allocator

Custom memory allocator (malloc) implemented in C.
salloc - secure allocator :)

> I've documented the process of building this allocator and what I learned on my [blog](https://strike24.github.io/posts/projects/building-a-memory-allocator/)
> Keep in mind, this may be slightly outdated as I update the allocator from time to time ;)

## How it works

A **"Segregated Bins"** allocator that uses a free list to manage allocated memory chunks.
The bins are organized by size classes, each containing a list of free aligned memory chunks, assuring O(1) allocation complexity (At least in the average case)
The allocator uses a **first-fit** strategy to find a simmliar sized chunk for allocation, and it splits larger chunks when necessary. When a chunk is freed, it is added back to the free list and is merged with adjacent free chunks if possible.

The allocator also includes a mutex lock to ensure thread safety during allocation and deallocation.

Heap Mitigations currently implemented:

- Safe Unlinking & Double Free:
  integrity checking of the free list pointers before unlinking to ensure they did not get altered, and checking if a chunk is already freed before adding it back to the free list, preventing double free vulnerabilities.
- Use-After-Free:
  overwrite freed chunks with garbage data to prevent use-after-free vulnerabilities.
- Heap Canaries ("Security Cookies"):
  canary value at the header of each chunk to detect buffer overflows. If the canary is altered, the allocator aborts the program.
  The canary value is generated using a random number generator at the start of the program and is unique for each run.
