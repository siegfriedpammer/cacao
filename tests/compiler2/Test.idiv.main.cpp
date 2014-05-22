#include <stdio.h>

extern "C" int idiv(int, int);

int main() {
    printf("%d\n", idiv(10, 5));
    printf("%d\n", idiv(11, 5));
    printf("%d\n", idiv(5, 5));
    printf("%d\n", idiv(3, 5));
    printf("%d\n", idiv(-10, 5));
    printf("%d\n", idiv(10, -5));
    printf("%d\n", idiv(-10, -5));
    printf("%d\n", idiv(0, 5));
    //printf("%d\n", idiv(5, 0));

    return 0;
}
