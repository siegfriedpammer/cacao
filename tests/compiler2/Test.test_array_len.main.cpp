#include <cstdio>
#include <inttypes.h>

#include "MockArray.hpp"

extern "C" int32_t test_array_len(void*);


int main() {
    printf("%d\n", test_array_len(MockArray<long>(10)));
    printf("%d\n", test_array_len(MockArray<long>(1)));
    printf("%d\n", test_array_len(MockArray<long>(0)));
    printf("%d\n", test_array_len(MockArray<long>(42)));
    return 0;
}
