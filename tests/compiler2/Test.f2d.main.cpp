#include <stdio.h>

extern "C" double f2d(float);

int main() {
	printf("%f\n", f2d(10));
    printf("%f\n", f2d(-10));
    printf("%f\n", f2d(10.12345));
    printf("%f\n", f2d(-10.12345));

    return 0;
}
