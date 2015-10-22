// only for debugging
#include <stdio.h>

#include "settings.h"
#include "submit.h"

skiplist token_list;
int nemails, ntokens_submitted;
size_t term_length;

/* submit TOKEN LENGTH
 * Submit TOKEN of length LENGTH to the list. */
void submit(char *t, size_t l) {
    int n, *p;

    p = skiplist_find(token_list, t, l);
    if (p) n = *p;
    else n = 0;
    ++n;
    skiplist_insert_copy(token_list, t, l, &n, sizeof n);
    term_length += l;
//fprintf(stderr, "submit #%d, %.*s\n", ntokens_submitted, (int)l, t);
    ++ntokens_submitted;
}
