/*
check 'foo qux bar' '{foo}{qux}{bar}'
check 'this, that' '{this}{that}'
check '!$^%$$' ''
check 'only £12.99' '{only}'
*/

#include <stdio.h>
#include <string.h>

#include "token.h"

void submit(char *s, size_t l) {
    putchar('{');
    while (l--)
        putchar(*s++);
    putchar('}');
}


int main(int argc, char **argv) {
    tokenize(argv[1], strlen(argv[1]), 0);
}
