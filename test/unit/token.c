/*
check 'foo qux bar' '{foo}{qux}{bar}'
check 'this, that' '{this}{that}'
check '!$^%$$' ''
check 'only £12.99' '{only}'
check 'LoOk! We fOlD CASE!!' '{look}{we}{fold}{case}'
check 'Emails@are.tokens too' '{emails@are.tokens}{too}'
check 'we skip 1968-11-16 dates and 9/11/2001 other dates' '{we}{skip}{dates}{and}{other}{dates}'
*/

#include <stdio.h>
#include <string.h>

#include "token.h"

void compose(char *s, size_t l) {
    putchar('{');
    while (l--)
        putchar(*s++);
    putchar('}');
}

void compose_reset(void) {
}

int main(int argc, char **argv) {
    tokenize(argv[1], strlen(argv[1]), 0);
}
