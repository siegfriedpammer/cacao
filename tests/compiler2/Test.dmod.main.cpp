#include <stdio.h>

extern "C" double dmod(double, double);

int main() {
    printf("%f\n", dmod(0.10, 0.05));
    printf("%f\n", dmod(0.11, 0.05));
    printf("%f\n", dmod(0.5, 0.5));
    printf("%f\n", dmod(0.3, 0.5));
    printf("%f\n", dmod(3.0, 0.5));
    printf("%f\n", dmod(3.3, 0.5));
    printf("%f\n", dmod(-0.10, 0.05));
    printf("%f\n", dmod(0.10, -0.05));
    printf("%f\n", dmod(-0.10, -0.05));
    printf("%f\n", dmod(-0.13, 0.05));
    printf("%f\n", dmod(0.13, -0.05));
    printf("%f\n", dmod(-0.13, -0.05));
    printf("%f\n", dmod(0, 0.5));
    //printf("%f\n", idiv(5, 0));

    return 0;
}
