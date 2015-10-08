#define _DEFAULT_SOURCE 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "read.h"

void tokenize(char *s, size_t l) {
    putchar('{');
    while (l--)
        putchar(*s++);
    putchar('}');
    putchar('\n');
}

void compose_reset(void) {
}

int main(int argc, char **argv) {
    _Bool from;
    FILE *in;

    in = fopen(argv[1], "r");
    from = strchr(argv[1], 'F') != 0;
    do {
        read_email(from, in, 0);
    } while (from && !feof(in));
    fclose(in);
}
