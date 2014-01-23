#include <cstdio>
#include <inttypes.h>

#include "MockArray.hpp"

extern "C" int32_t BoyerMoore(void* pattern,void* text, void* matches, void* rightMostIndexes);


int main() {

	MockArray<int32_t> pattern(3);
	MockArray<int32_t> text(7);
	MockArray<int32_t> matches(7);
	MockArray<int32_t> rightMostIndexes(256);

	pattern[0] = (int32_t)'a';
	pattern[1] = (int32_t)'n';
	pattern[2] = (int32_t)'a';

	text[0] = (int32_t)'b';
	text[1] = (int32_t)'a';
	text[2] = (int32_t)'n';
	text[3] = (int32_t)'a';
	text[4] = (int32_t)'n';
	text[5] = (int32_t)'a';
	text[6] = (int32_t)'s';

	printf("pattern: ");
	for (int i = 0; i < pattern.size(); i ++) {
	  printf("%c", (char)pattern[i]);
	}
	printf("\n");
	printf("text: ");
	for (int i = 0; i < text.size(); i ++) {
	  printf("%c", (char)text[i]);
	}
	printf("\n");

	int32_t num_matches = BoyerMoore(pattern, text, matches, rightMostIndexes);

	printf("Matches: %d\n", num_matches);
	for (int i = 0; i < text.size(); i ++) {
	  printf("%c", (char)text[i]);
	}
	printf("\n");
	for (int i = 0; i < num_matches; i ++) {
	  for (int j = 0; j < matches[i]; ++j) {
        printf(" ");
	  }
	  for (int i = 0; i < pattern.size(); i ++) {
		printf("%c", (char)pattern[i]);
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
