#include <stdio.h>

extern "C" float d2f(double);

int main() {
	printf("%f\n", d2f(10));
    printf("%f\n", d2f(-10));
    printf("%f\n", d2f(10.12345));
    printf("%f\n", d2f(-10.12345));
    printf("%f\n", d2f(4.9E-324));
    printf("%f\n", d2f(1.7976931348623157E308));
    printf("%f\n", d2f(-1.7976931348623157E308));

    return 0;
}
