#include <cstdio>
#include <inttypes.h>

#include "MockArray.hpp"

extern "C" void matrixMult(void*,void*,void*);


int main() {
	const int32_t n = 4;
	const int32_t m = 2;
	const int32_t p = 3;

	int32_t a[n][m] = {
		{ 1, 2},
		{ 3, 4},
		{ 5, 6},
		{ 7, 8}
	};
	int32_t b[m][p] = {
		{3, 6, 9},
		{4, 8,12}
	};

	MockArray2dim<int32_t> A(n,m,(int32_t*)a);
	MockArray2dim<int32_t> B(m,p,(int32_t*)b);
	MockArray2dim<int32_t> C(n,p);

	printf("A:\n");
	for (int32_t i = 0; i < n; ++i) {
		for (int32_t j = 0; j < m; ++j) {
			printf("%03d ", A[i][j]);
		}
		printf("\n");
	}
	printf("\n");
	printf("B:\n");
	for (int32_t i = 0; i < m; ++i) {
		for (int32_t j = 0; j < p; ++j) {
			printf("%03d ", B[i][j]);
		}
		printf("\n");
	}
	printf("\n");
	matrixMult(A,B,C);
	printf("result:\n");
	for (int32_t i = 0; i < n; ++i) {
		for (int32_t j = 0; j < p; ++j) {
			printf("%03d ", C[i][j]);
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
