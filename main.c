#include "stdio.h"
#include "allocator.h"
#include <stdio.h>

int main()
{
    char *str = (char *)salloc(8);
    if (str == NULL)
    {
        fprintf(stderr, "Allocation failed!\n");
        return 1;
    }

    fgets(str, 100, stdin);
    printf("Data: %s", str);

    printf("Pointer to allocated memory: %p\n\n", str);

    sfree(str);
    printf("Data after free: %s\n", str);
}

// int main()
// {
//     char *str = (char *)salloc(64);
//     if (str == NULL)
//     {
//         fprintf(stderr, "Allocation failed!\n");
//         return 1;
//     }

//     strcpy(str, "Hello Allocator!");
//     printf("Data: %s\n", str);
//     print_debug();

//     str = srealloc(str, 16);
//     printf("Data after realloc: %s\n", str);
//     print_debug();

//     sfree(str);
//     printf("Data after free: %s\n", str);
// }

// int main()
// {
//     // canary test
//     char *str = (char *)salloc(16);
//     char *str2 = (char *)salloc(16);

//     strcpy(str, "AAAABBBBCCCCDDDDEEEEFFFFGGGGHHHH");
//     sfree(str2);
//     sfree(str);
// }
