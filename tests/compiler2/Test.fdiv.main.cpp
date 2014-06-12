#include <stdio.h>

extern "C" float fdiv(float, float);

int main() {
    printf("%f\n", fdiv(2.0, 2.3));
    printf("%f\n", fdiv(-3.2, 5.8));
    printf("%f\n", fdiv(-3.2, -5.8));
    printf("%f\n", fdiv(1.99234, 1.24234));

    return 0;
}
