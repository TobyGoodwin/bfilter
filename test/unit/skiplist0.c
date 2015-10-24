/*
depends test/uskiplist0
export BFILTER_DB=$(mktemp)
mcheck 'one=>1,three=>3,two=>2,' one 1 two 2 three 3
mcheck 'one=>9,three=>8,two=>7,' one 9 two 7 three 8
*/

#include <stdio.h>
#include <string.h>

#include "skiplist.h"

void compose(char *s, size_t l) {
    putchar('{');
    while (l--)
        putchar(*s++);
    putchar('}');
}

void compose_reset(void) {
}

int main(int argc, char **argv) {
    int i;
    skiplist s = skiplist_new(0);
    skiplist_iterator x;

    for (i = 1; i < argc; i += 2) {
        char *k = argv[i];
        char *v = argv[i + 1];

        skiplist_insert_copy(s, k, strlen(k) + 1, v, strlen(v) + 1);
    }

    for (x = skiplist_itr_first(s); x; x =skiplist_itr_next(s, x))
        printf("%s=>%s,", (char *)skiplist_itr_key(s, x, 0),
                (char *)skiplist_itr_value(s, x));
    printf("\n");
}
