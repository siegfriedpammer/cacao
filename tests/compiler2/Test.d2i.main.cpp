#include <stdio.h>

extern "C" int d2i(double);

int main() {

	printf("%d\n", d2i(10));
    printf("%d\n", d2i(-10));
    printf("%d\n", d2i(10.12345));
    printf("%d\n", d2i(-10.12345));
    printf("%d\n", d2i(1.7976931348623157E308));
    printf("%d\n", d2i(-1.7976931348623157E308));

    return 0;
}
