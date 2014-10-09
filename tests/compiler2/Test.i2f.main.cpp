#include <stdio.h>

extern "C" float i2f(int);

int main() {
    printf("%f\n", i2f(2147483647));

    // Integer.MAX_VALUE - 30
    printf("%f\n", i2f(2147483617));

    // Integer.MIN_VALUE
    printf("%f\n", i2f(-2147483648));

    // Integer.MIN_VALUE + 30
    printf("%f\n", i2f(-2147483618));

    return 0;
}
