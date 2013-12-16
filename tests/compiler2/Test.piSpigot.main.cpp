#include <stdio.h>

extern "C" double piSpigot(long num_digits);

extern "C" double piSpigot_c(long num_digits) {
    double pi = 0;
    for (long i = 0; i < num_digits; ++i) {
        // fake power
        long p =1;
        for (long j = 0; j < i; ++j) {
            p *= 16;
        }
        long d1 = 8*i + 1;
        long d2 = 8*i + 4;
        long d3 = 8*i + 5;
        long d4 = 8*i + 6;

        pi += 1/(double)p * ( 4/(double)d1 - 2/(double)d2
            - 1/(double)d3 - 1/(double)d4 );
    }
    return pi;
}

int main() {
    int i;
    for(i=0; i < 15 ; ++i) {
        printf("%.50g\n", piSpigot(i));
    }
    return 0;
}
