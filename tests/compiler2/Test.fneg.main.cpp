#include <stdio.h>

extern "C" float fneg(float);

int main() {
    printf("%f\n", fneg(-42.0));
    printf("%f\n", fneg(42.0));
    printf("%f\n", fneg(0.0));
    printf("%f\n", fneg(0.0/0.0));

    return 0;
}
