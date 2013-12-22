#include <stdio.h>

extern "C" long Min(long,long);


int main() {
    long x[][2] = {
      {-1, 1},
      { 1,-1},
      { 0, 0}
    };
    for(long i = 0; !(x[i][0] == 0 && x[i][1] == 0); ++i) {
        printf("Min(%ld,%ld) = %ld\n", x[i][0], x[i][1], Min(x[i][0],x[i][1]));
    }
    return 0;
}
