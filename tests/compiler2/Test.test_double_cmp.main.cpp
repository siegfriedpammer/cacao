#include <stdio.h>

extern "C" int test_double_cmp(double,double);

int main() {
    printf("%d\n", test_double_cmp(-1.0, 1.0));
    printf("%d\n", test_double_cmp( 1.0,-1.0));
    printf("%d\n", test_double_cmp( 0.0, 0.0));
    printf("%d\n", test_double_cmp( 0.0/0.0, 0.0));
    printf("%d\n", test_double_cmp( 0.0, 0.0/0.0));
    printf("%d\n", test_double_cmp( 0.0/0.0, 0.0/0.0));

    return 0;
}
