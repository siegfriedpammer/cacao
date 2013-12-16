#include <cstdio>
#include <inttypes.h>

#include "MockArray.hpp"

extern "C" void test_array_2dim_store(void*);


int main() {
    MockArray2dim<int> a(10,10);
	test_array_2dim_store(a);
	assert(a[0].size() == 10);
	for (int i = 0; i < 10; ++i) {
		for (int j = 0; j < 10; ++j) {
			printf("%d\n", a[i][j]);
		}
	}
    return 0;
}
