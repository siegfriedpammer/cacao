#include <stdio.h>

long fact(long);


int main() {
    printf("%2s %10s\n", "n", "n!");
    for(long i = 0; i < 10; ++i)
        printf("%2ld %10ld\n", i, fact(i));
    return 0;
}
