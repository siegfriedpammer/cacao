#include <stdio.h>

extern "C" long lneg(long);

int main() {
    printf("%ld\n", lneg(-42));
    printf("%ld\n", lneg(42));
    printf("%ld\n", lneg(0));

    return 0;
}
