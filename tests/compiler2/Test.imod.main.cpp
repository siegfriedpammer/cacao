#include <stdio.h>

extern "C" int imod(int, int);

int main() {
    printf("%d\n", imod(10, 5));
    printf("%d\n", imod(11, 5));
    printf("%d\n", imod(5, 5));
    printf("%d\n", imod(3, 5));
    printf("%d\n", imod(-10, 5));
    printf("%d\n", imod(10, -5));
    printf("%d\n", imod(-10, -5));
    printf("%d\n", imod(-13, 5));
    printf("%d\n", imod(13, -5));
    printf("%d\n", imod(-13, -5));
    printf("%d\n", imod(0, 5));
    //printf("%d\n", idiv(5, 0));

    return 0;
}
