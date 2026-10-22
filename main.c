#include "stdio.h"
#include "allocator.h"
#include <stdio.h>

// canary and overflow testing
// int main()
// {
//     char *str = (char *)salloc(64);
//     char *str2 = (char *)salloc(64);

//     if (str == NULL)
//     {
//         fprintf(stderr, "Allocation failed!\n");
//         return 1;
//     }

//     fgets(str, 100, stdin);
//     printf("Data: %s", str);

//     printf("Pointer to allocated memory: %p\n\n", str);

//     // srealloc(str, 16);
//     // printf("Data after being reallocated to 16: %s\n", str);

//     sfree(str);
//     sfree(str2);
//     printf("Data after free: %s\n", str);

//     tcache_print();
// }

// tcache testing
int main()
{
    printf("Before any allocations:\n");
    tcache_print();
    char *str = (char *)salloc(64);
    char *str2 = (char *)salloc(64);
    char *str3 = (char *)salloc(500);
    char *str4 = (char *)salloc(490);

    printf("After allocations:\n");
    tcache_print();

    sfree(str);
    printf("After freeing str1:\n");
    tcache_print();

    sfree(str2);
    sfree(str3);
    sfree(str4);
    printf("After freeing str2,3,4:\n");
    tcache_print();

    salloc(64);
    printf("After allocating 64 bytes again:\n");
    tcache_print();
}