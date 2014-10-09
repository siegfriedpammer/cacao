#include <stdio.h>

extern "C" int f2i(float);

int main() {
	printf("%d\n", f2i(10));
    printf("%d\n", f2i(-10));
    printf("%d\n", f2i(10.12345));
    printf("%d\n", f2i(-10.12345));

    return 0;
}
