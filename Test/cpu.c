#include <stdio.h>
#include <stdlib.h>

int main() {
    long size = 1024 * 1024 * 256;
    long *dynamicArray = (long *)malloc(size * sizeof(long));
    
    for(long i = 0; i < size; i++)
    {
        dynamicArray[i] = i;
    }

    for(;;);
    return 0;
}
