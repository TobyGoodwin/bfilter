/*
check 'foo qux bar' '{foo}{qux}{bar}'
check 'this, that' '{this}{that}'
check '!$^%$$' ''
check 'only £12.99' '{only}'
*/

#define _DEFAULT_SOURCE 1

#include <stdio.h>
#include <stdlib.h>

#include "bfilter.h"

void tokenize(char *s, size_t l) {
    while (l--)
        putchar(*s++);
    putchar('\n');
}


int main(int argc, char **argv) {
    FILE *fh;

    read_email(0, 0, stdin, &fh);
}
