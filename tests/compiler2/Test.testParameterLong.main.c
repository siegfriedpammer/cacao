#include <stdio.h>

long testParameterLong(long a, long b, long c, long d, long e, long f, long g, long h, long i);

int main() {
    printf("%ld\n", testParameterLong(1,2,3,4,5,6,7,8,9));
    printf("%ld\n", testParameterLong(1,0,0,0,0,0,0,0,0));
    printf("%ld\n", testParameterLong(0,1,0,0,0,0,0,0,0));
    printf("%ld\n", testParameterLong(0,0,1,0,0,0,0,0,0));
    printf("%ld\n", testParameterLong(0,0,0,1,0,0,0,0,0));
    printf("%ld\n", testParameterLong(0,0,0,0,1,0,0,0,0));
    printf("%ld\n", testParameterLong(0,0,0,0,0,1,0,0,0));
    printf("%ld\n", testParameterLong(0,0,0,0,0,0,1,0,0));
    printf("%ld\n", testParameterLong(0,0,0,0,0,0,0,1,0));
    printf("%ld\n", testParameterLong(0,0,0,0,0,0,0,0,1));
    return 0;
}
