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

    srealloc(str, 16);
    printf("Data after being reallocated to 16: %s\n", str);

    sfree(str);
    printf("Data after free: %s\n", str);
}
