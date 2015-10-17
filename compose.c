// only for debugging
#include <stdio.h>

#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "compose.h"
#include "settings.h"
#include "skiplist.h"
#include "submit.h"

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
    char term[(MAX_TERM_LEN + 1) * HISTORY_LEN];
    int n;

    for (n = 1; n <= ntokens_history; ++n) {
        char *p;
        int i;
        for (i = 0, p = term; i < n; ++i) {
            int j;
            if (i > 0) *(p++) = '%';
            j = (history_index - n + i + HISTORY_LEN) % HISTORY_LEN;
            memcpy(p, token_history[j].term, token_history[j].len);
            p += token_history[j].len;
        }

        submit(term, p - term);
    }
}

/* submit_token TERM LENGTH
 * Submit an individual LENGTH-character TERM to the list of known tokens. */
void compose(uint8_t *term, size_t len) {
    /* Update history. */
    memcpy(token_history[history_index].term, term, len);
    token_history[history_index].len = len;
    history_index = (history_index + 1) % HISTORY_LEN;
    if (ntokens_history < HISTORY_LEN)
        ++ntokens_history;

    /* Submit this token and composites with preceding ones. */
    record_tokens();
}

void compose_reset(void) {
    ntokens_submitted = 0;
    ntokens_history = 0;
}

