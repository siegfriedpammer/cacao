#include <cstdio>
#include <inttypes.h>

#include "MockArray.hpp"

extern "C" int32_t test_array_load(void*, int32_t);


int main() {
    MockArray<long> a(10);
	for (int i = 0; i < 10; ++i) {
		a[i] = i;
	}
	for (int i = 0; i < 10; ++i) {
		printf("%d\n", test_array_load(a,i));
	}
    return 0;
}
