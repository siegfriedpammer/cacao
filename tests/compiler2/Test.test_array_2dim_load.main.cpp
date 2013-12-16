#include <cstdio>
#include <inttypes.h>

#include "MockArray.hpp"

extern "C" int32_t test_array_2dim_load(void*,int32_t,int32_t);


int main() {
    MockArray2dim<int> a(10,10);
	for (int i = 0; i < 10; ++i) {
		for (int j = 0; j < 10; ++j) {
			int &entry = a[i][j];
			entry = i * j;
		}
	}
	for (int i = 0; i < 10; ++i) {
		for (int j = 0; j < 10; ++j) {
			printf("%d\n", test_array_2dim_load(a,i,j));
		}
	}
    return 0;
}
