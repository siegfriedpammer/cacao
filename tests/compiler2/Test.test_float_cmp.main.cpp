#include <stdio.h>

extern "C" int test_float_cmp(float,float);

int main() {
    printf("%d\n", test_float_cmp(-1.0, 1.0));
    printf("%d\n", test_float_cmp( 1.0,-1.0));
    printf("%d\n", test_float_cmp( 0.0, 0.0));
    printf("%d\n", test_float_cmp( 0.0/0.0, 0.0));
    printf("%d\n", test_float_cmp( 0.0, 0.0/0.0));
    printf("%d\n", test_float_cmp( 0.0/0.0, 0.0/0.0));

    return 0;
}
