#include <stdio.h>

extern "C" double testParameterDouble(double a, double b, double c, double d, double e, double f, double g, double h, double i);

int main() {
    printf("%g\n", testParameterDouble(.1,.2,.3,.4,.5,.6,.7,.8,.9));
    return 0;
}
