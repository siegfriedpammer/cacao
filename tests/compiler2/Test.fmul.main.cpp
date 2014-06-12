#include <stdio.h>

extern "C" float fmul(float, float);

int main() {
    printf("%f\n", fmul(2.0, 2.3));
    printf("%f\n", fmul(-3.2, 5.8));
    printf("%f\n", fmul(-3.2, -5.8));
    printf("%f\n", fmul(1.99234, 1.24234));

    return 0;
}
