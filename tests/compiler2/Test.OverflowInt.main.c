#include <stdio.h>
#include <inttypes.h>

int32_t OverflowInt(int32_t);


int main() {
    printf("%d\n", OverflowInt(15));
    return 0;
}
