#include <stdio.h>

extern "C" long testParameterLong3(long a, long b, long c, long d, long e, long f, long g, long h, long i);

int main() {
    printf("%ld\n", testParameterLong3(1,2,3,4,5,6,7,8,9));
    return 0;
}
