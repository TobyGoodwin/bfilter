#include <stdio.h>

#define CLAMP0 (0.01)
#define CLAMP1 (1 - CLAMP0)
int main(void) {
    int i, j;
    double a, b, s;

    for (i = 0; i < 16; ++i) {
        a = b = 1.;
        for (j = 0; j < i; ++j) {
            a *= CLAMP0;
            b *= 1. - CLAMP0;
        }
        for (j = i; j < 15; ++j) {
            a *= CLAMP1;
            b *= 1. - CLAMP1;
        }
        s = a / (a + b);
        printf("%2d %f\n", i, s);
    }
}


