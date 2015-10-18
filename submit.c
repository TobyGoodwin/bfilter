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
    struct wordcount *pw;

    pw = skiplist_find(token_list, t, l);
    if (pw) {
        if (pw->nemail < nemails) {
            pw->nemail = nemails;
            ++pw->n;
        }
    } else {
        struct wordcount w = { 0 };
        w.nemail = nemails;
        w.n = 1;
        skiplist_insert_copy(token_list, t, l, &w, sizeof w);
        term_length += l;
//fprintf(stderr, "submit #%d, %.*s\n", ntokens_submitted, (int)l, t);
        ++ntokens_submitted;
    }
}
