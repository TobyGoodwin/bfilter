/*
check 'foo qux bar' '{foo}{qux}{bar}'
check 'this, that' '{this}{that}'
check '!$^%$$' ''
check 'only £12.99' '{only}'
*/

#define _DEFAULT_SOURCE 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bfilter.h"

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
    _Bool from, pass;
    FILE *in, *out;

    in = fopen(argv[1], "r");
    from = strchr(argv[1], 'F') != 0;
    pass = strchr(argv[1], 'P') != 0;
    read_email(from, pass, in, &out);
    if (pass) {
        char buf[1024];
        size_t n;

        rewind(out);
        while ((n = fread(buf, 1, 1024, out)))
            fwrite(buf, 1, n, stdout);
        fclose(out);
    }
    fclose(in);
}
