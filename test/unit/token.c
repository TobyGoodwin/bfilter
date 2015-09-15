/*
echo foo
check 'foo qux bar' '{foo}{qux}{bar}'
*/

#include <stdio.h>
#include <string.h>

#include "token.h"

void token_print(char *s, size_t l) {
    putchar('{');
    while (l--)
        putchar(*s++);
    putchar('}');
}


int main(int argc, char **argv) {
    tokenize(argv[1], strlen(argv[1]), 0, token_print);
}
