// only for debugging
#include <stdio.h>

#include <string.h>
#include <unistd.h>

#include "bfilter.h"
#include "submit.h"

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
