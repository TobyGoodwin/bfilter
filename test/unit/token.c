/*
depends test/utoken
check 'foo qux bar' '{foo}{qux}{bar}'
check 'this, that' '{this}{that}'
check '!$^%$$' ''
check 'only £12.99' '{only}{£12.99}'
check "LoOk! We don't fOlD CASE!!" "{LoOk}{We}{don't}{fOlD}{CASE}"
check "she said 'hello'" '{she}{said}{hello}'
check 'Emails@are.tokens too' '{Emails@are.tokens}{too}'
check 'we skip 1968-11-16 dates and 9/11/2001 other dates' '{we}{skip}{dates}{and}{other}{dates}'
check 'we skip <em>any</em> html markup' '{we}{skip}{any}{html}{markup}'
check '#start -with .punctuation' '{start}{-with}{punctuation}'
*/

#include <stdio.h>
#include <stdint.h>
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
    tokenize((uint8_t *)argv[1], strlen(argv[1]), 0);
}
