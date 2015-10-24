/*
depends test/uskiplist1
export BFILTER_DB=$(mktemp)
mcheck 'one=>1,three=>1,two=>1,' one two three
mcheck 'five=>1,four=>1,one=>2,three=>1' three one four one five
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

    for (i = 1; i < argc; ++i) {
        int n, *pn;
        char *k = argv[i];

        pn = skiplist_find(s, k, strlen(k) + 1);
        if (pn) n = *pn;
        else n = 0;

        ++n;
        skiplist_insert_copy(s, k, strlen(k) + 1, &n, sizeof n);
    }

    for (x = skiplist_itr_first(s); x; x =skiplist_itr_next(s, x))
        printf("%s=>%d,", (char *)skiplist_itr_key(s, x, 0),
                *(int *)skiplist_itr_value(s, x));
    printf("\n");
}
