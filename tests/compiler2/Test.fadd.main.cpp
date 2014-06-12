#include <stdio.h>

extern "C" float fadd(float, float);

int main() {
    printf("%f\n", fadd(2.0, 2.3));
    printf("%f\n", fadd(-3.2, 5.8));
    printf("%f\n", fadd(-3.2, -5.8));
    printf("%f\n", fadd(1.99234, 1.24234));

    return 0;
}
