#include <stdio.h>

extern "C" char i2c(int);

int main() {

	printf("%d\n", i2c(10));
	printf("%d\n", i2c(-10));

	// Integer.MAX_VALUE
    printf("%d\n", i2c(2147483647));

    // Integer.MIN_VALUE
    printf("%d\n", i2c(-2147483648));

    printf("%d\n", i2c(128));
    printf("%d\n", i2c(-129));

    return 0;
}
