#include <cstdio>
#include <inttypes.h>

#include "MockArray.hpp"

extern "C" void test_double_array_store(void*);


int main() {
    MockArray<double> a(10);
        test_double_array_store(a);
	for (int i = 0; i < 10; ++i) {
		printf("%g\n", a[i]);
	}
    return 0;
}
