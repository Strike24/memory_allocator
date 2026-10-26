#include "allocator.h"
#include <stdio.h>
#include "stdint.h"
// simple tcache next pointer poisoning

int main()
{

    volatile int password = 123456;

    int *A = salloc(64);
    int *B = salloc(64);

    sfree(B);
    sfree(A);

    // use after free

    printf("Password variable addr: %p\n", &password);

    printf("Write into *A (A->next):\n");
    scanf("%lx", (uintptr_t *)A);

    for (int i = 0; i < 2; i++)
    {
        char *alloc = (char *)salloc(64);
        printf("#%d: %p\n\n", i + 1, alloc);
    }
}