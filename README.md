# Memory Allocator

Custom memory allocator (malloc) implemented in C.
balloc - bad allocator :)
Work in progress

## How it works

A binning allocator that uses a free list to manage allocated memory chunks.
The bins are organized by size classes, each containing a list of free memory chunks, assuring O(1) allocation complexity (At least in the average case).

The free list are doubly linked, designed to save memory by using a union to store the previous and next pointers only when the chunk is free. When the chunk is allocated, the data overrides them.
