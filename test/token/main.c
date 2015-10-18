#include <stdint.h>
#include <stdio.h>

#include "read.h"

void compose(uint8_t *t, size_t l) {
    printf("%.*s\n", (int)l, t);
}

int main(int argc, char **argv) {
    FILE *in;

    in = fopen(argv[1], "r");
    read_email(0, in, 0);
    fclose(in);
}
