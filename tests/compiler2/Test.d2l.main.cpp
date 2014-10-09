#include <stdio.h>

extern "C" long d2l(double);

int main() {

	printf("%ld\n", d2l(10));
    printf("%ld\n", d2l(-10));
    printf("%ld\n", d2l(10.12345));
    printf("%ld\n", d2l(-10.12345));
    printf("%ld\n", d2l(1.7976931348623157E308));
    printf("%ld\n", d2l(-1.7976931348623157E308));

    return 0;
}
