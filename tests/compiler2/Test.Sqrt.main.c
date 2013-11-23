#include <stdio.h>

long Sqrt(long);


int main() {
    printf("%2s %10s\n", "n", "n!");
    for(long i = 0; i < 1000; ++i)
        printf("%2ld %10ld\n", i, Sqrt(i));
    return 0;
}
