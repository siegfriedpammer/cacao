#include <stdio.h>

extern "C" char i2b(int);

int main() {

	printf("%d\n", i2b(10));
	printf("%d\n", i2b(-10));

	// Integer.MAX_VALUE
    printf("%d\n", i2b(2147483647));

    // Integer.MIN_VALUE
    printf("%d\n", i2b(-2147483648));

    printf("%d\n", i2b(128));
    printf("%d\n", i2b(-129));

    return 0;
}
