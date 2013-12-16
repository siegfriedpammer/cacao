#include <cstdio>
#include <inttypes.h>

#include "MockArray.hpp"

extern "C" float test_float_array_load(void*, int32_t);


int main() {
    MockArray<float> a(10);
	for (int i = 0; i < 10; ++i) {
		a[i] = i;
	}
	for (int i = 0; i < 10; ++i) {
		printf("%f\n", test_float_array_load(a,i));
	}
    return 0;
}
