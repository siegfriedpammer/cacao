#include <cstdio>
#include <inttypes.h>

#include "MockArray.hpp"

extern "C" void permut(void*,void*,void*);

template<unsigned n>
struct fact {
  enum { value = n * fact<n-1>::value };
};

template<>
struct fact<0> {
  enum { value = 1 };
};

int main() {
	const int32_t n = 5;
	const int32_t fn = fact<n>::value;

	MockArray<int32_t> a(n);
	MockArray<int32_t> p(n);
	MockArray2dim<int32_t> result(fn,n);

	for (int32_t i = 0; i < n; ++i) {
		a[i] = i;
		p[i] = 0;
	}
	permut(a,p,result);
	printf("permutations:\n");
	for (int32_t i = 0; i < fn; ++i) {
		for (int32_t j = 0; j < n; ++j) {
			printf("%03d ", result[i][j]);
		}
		printf("\n");
	}
	return 0;
}

/*
 * These are local overrides for various environment variables in Emacs.
 * Please do not remove this and leave it at the end of the file, where
 * Emacs will automagically detect them.
 * ---------------------------------------------------------------------
 * Local variables:
 * mode: c++
 * indent-tabs-mode: t
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 * vim:noexpandtab:sw=4:ts=4:
 */
