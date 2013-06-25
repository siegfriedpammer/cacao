#include <stdio.h>
#include <inttypes.h>

int64_t OverflowLongIntLong(int64_t);


int main() {
    printf("%ld\n", OverflowLongIntLong(15));
    return 0;
}
