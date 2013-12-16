#include <stdio.h>

extern "C" double pi();

int main() {
    printf("%g\n", pi());
    return 0;
}
