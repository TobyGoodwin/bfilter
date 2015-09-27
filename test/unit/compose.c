/*
depends test/ucompose
mcheck '{hello}{world}{hello%world}' hello world
*/

#include <stdio.h>
#include <string.h>

#include "compose.h"

void submit(char *s, size_t l) {
    putchar('{');
    while (l--)
        putchar(*s++);
    putchar('}');
}

int main(int argc, char **argv) {
    int i;

    for (i = 1; i < argc; ++i) {
        compose(argv[i], strlen(argv[i]));
    }
}
