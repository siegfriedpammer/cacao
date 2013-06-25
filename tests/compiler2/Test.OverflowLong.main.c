#include <stdio.h>

long OverflowLong(long);


int main() {
    printf("%ld\n", OverflowLong(15));
    return 0;
}
