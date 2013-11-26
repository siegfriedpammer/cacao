#include <stdio.h>

extern "C" long power(long,long);

int main() {
    printf("%2s %2s %10s\n", "n", "x", "n^x");
    for(long i = 0; i < 10; ++i)
        for(long j = 0; j < 10; ++j)
            printf("%2ld %2ld %10ld\n", i , j, power(i,j));
    return 0;
}
