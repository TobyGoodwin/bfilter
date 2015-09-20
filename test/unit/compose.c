/*
depends test/ucompose
check foo bar
*/

#include <stdio.h>

#include "compose.h"

void submit(char *s, size_t l) {
    putchar('{');
    while (l--)
        putchar(*s++);
    putchar('}');
}

int main(void) {
    compose("hello", 5);
    compose("world", 5);
}
