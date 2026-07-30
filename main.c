#include "stdio.h"
#include "allocator.h"

int main()
{
    char *str = (char *)balloc(64);
    if (str == NULL)
    {
        fprintf(stderr, "Allocation failed!\n");
        return 1;
    }

    strcpy(str, "Hello Allocator!");
    printf("Data: %s\n", str);
    print_debug();

    str = brealloc(str, 16);
    printf("Data after realloc: %s\n", str);
    print_debug();

    bfree(str);
    printf("Data after free: %s\n", str);
}
