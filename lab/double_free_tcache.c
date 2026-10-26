#include "allocator.h"
#include <stdio.h>
// simple tcache double free demonstration

int main()
{
    char *pointer = (char *)salloc(64);
    printf("First allocation: 64 bytes, %p\n\n", pointer);

    sfree(pointer);
    printf("sfree(%p) - #1\n\n", pointer);

    sfree(pointer);
    printf("sfree(%p) - #2\n\n", pointer);

    // allocate 2 *different* allocations
    for (int i = 0; i < 2; i++)
    {
        char *alloc = (char *)salloc(64);
        printf("#1: %p\n", alloc);
    }

    printf("Same address, 2 different allocations");
}