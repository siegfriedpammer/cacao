#include <stdio.h>

extern "C" long f2l(float);

int main() {
	printf("%ld\n", f2l(10.0f));
    printf("%ld\n", f2l(-10.0f));
    printf("%ld\n", f2l(10.12345f));
    printf("%ld\n", f2l(-10.12345f));

    return 0;
}
