#include <stdio.h>

extern "C" float frem(float, float);

int main() {
	printf("%f\n", frem(0, 1.0));
    printf("%f\n", frem(2.0, 1.0));
    printf("%f\n", frem(2.3, 1.0));
    printf("%f\n", frem(-3.0, 1.0));
    printf("%f\n", frem(-3.2, 1.0));

    // should return 2.0 as we use fprem and not fprem1
    // fprem rounds towards 0, while fprem1 rounds towards
    // the next integer
    printf("%f\n", frem(6.5, 2.25));

    return 0;
}
