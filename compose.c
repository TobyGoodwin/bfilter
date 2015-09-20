// only for debugging
#include <stdio.h>

#include <string.h>
#include <unistd.h>

#include "settings.h"
#include "skiplist.h"
#include "compose.h"

skiplist wordlist;
int nemails;
size_t termlength;

int ntokens_submitted;
int history_index, ntokens_history;

struct thist {
    unsigned char term[MAX_TERM_LEN];
    size_t len;
};
struct thist token_history[HISTORY_LEN];

/* record_tokens
 * Record the most recently submitted token, and composite tokens from the
 * history. */
void record_tokens(void) {
    unsigned char term[(MAX_TERM_LEN + 1) * HISTORY_LEN];
    struct wordcount *pw;
    int n;

    for (n = 1; n <= ntokens_history; ++n) {
        unsigned char *p;
        int i;
        for (i = 0, p = term; i < n; ++i) {
            int j;
            if (i > 0) *(p++) = '%';
            j = (history_index - n + i + HISTORY_LEN) % HISTORY_LEN;
            memcpy(p, token_history[j].term, token_history[j].len);
            p += token_history[j].len;
        }

        pw = skiplist_find(wordlist, term, p - term);
        if (pw) {
            if (pw->nemail < nemails) {
                pw->nemail = nemails;
                ++pw->n;
            }
        } else {
            struct wordcount w = { 0 };
            w.nemail = nemails;
            w.n = 1;
            skiplist_insert_copy(wordlist, term, p - term, &w, sizeof w);
            termlength += p - term;
            ++ntokens_submitted;
        }
    }
}

/* submit_token TERM LENGTH
 * Submit an individual LENGTH-character TERM to the list of known tokens. */
void submit(char *term, size_t len) {
    /* Update history. */
    memcpy(token_history[history_index].term, term, len);
    token_history[history_index].len = len;
    history_index = (history_index + 1) % HISTORY_LEN;
    if (ntokens_history < HISTORY_LEN)
        ++ntokens_history;

    /* Submit this token and composites with preceding ones. */
    record_tokens();
}

void submit_reset(void) {
    ntokens_submitted = 0;
    ntokens_history = 0;
}

