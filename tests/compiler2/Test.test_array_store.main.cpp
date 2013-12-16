#include <cstdio>
#include <inttypes.h>

#include "MockArray.hpp"

extern "C" void test_array_store(void*, int32_t, int64_t);


int main() {
    MockArray<long> a(10);
	for (int i = 0; i < 10; ++i) {
		a[i] = i;
		test_array_store(a,i,i);
	}
	for (int i = 0; i < 10; ++i) {
		printf("%ld\n", a[i]);
	}
    return 0;
}
