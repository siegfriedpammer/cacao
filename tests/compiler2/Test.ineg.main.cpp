#include <stdio.h>

extern "C" int ineg(int);

int main() {
    printf("%d\n", ineg(-42));
    printf("%d\n", ineg(42));
    printf("%d\n", ineg(0));

    return 0;
}
