#include <stdio.h>

extern "C" short i2s(int);

int main() {

	printf("%d\n", i2s(10));
	printf("%d\n", i2s(-10));

	// Integer.MAX_VALUE
    printf("%d\n", i2s(2147483647));

    // Integer.MIN_VALUE
    printf("%d\n", i2s(-2147483648));

    printf("%d\n", i2s(32768));
    printf("%d\n", i2s(-32769));

    return 0;
}
