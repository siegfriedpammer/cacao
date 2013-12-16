#include <stdio.h>

extern "C" double dneg(double);

int main() {
    printf("%g\n", dneg(-42.0));
    printf("%g\n", dneg(42.0));
    printf("%g\n", dneg(0.0));
    printf("%g\n", dneg(0.0/0.0));

    return 0;
}
