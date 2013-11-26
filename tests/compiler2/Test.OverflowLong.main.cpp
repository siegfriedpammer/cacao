#include <stdio.h>
#include <inttypes.h>

extern "C" int64_t OverflowLong(int64_t);


int main() {
    printf("%ld\n", OverflowLong(15));
    return 0;
}
