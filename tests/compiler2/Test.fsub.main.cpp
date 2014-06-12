#include <stdio.h>

extern "C" float fsub(float, float);

int main() {
    printf("%f\n", fsub(2.0, 2.3));
    printf("%f\n", fsub(-3.2, 5.8));
    printf("%f\n", fsub(-3.2, -5.8));
    printf("%f\n", fsub(1.99234, 1.24234));

    return 0;
}
