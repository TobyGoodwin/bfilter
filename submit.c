// only for debugging
#include <stdio.h>

#include <string.h>
#include <unistd.h>

#include "bfilter.h"
#include "submit.h"

/* submit_token TOKEN LENGTH
 * Submit an individual LENGTH-character TOKEN to the list of known tokens. */
void submit(char *tok, size_t len) {
    if (len < 2 || ntokens_submitted > MAX_TOKENS)
        return;
    else if (len > 16 && strncmp(tok, "--", 2) == 0)
        return; /* probably a MIME separator */
    else {
        unsigned char term[MAX_TERM_LEN];
        int i, has_alpha = 0;
        
        /* Discard long terms, dates, numbers other than IP numbers. */
        if (len > MAX_TERM_LEN)
            len = MAX_TERM_LEN;

	fprintf(stderr, "%.*s ", (int)len, tok);
        for (i = 0; i < len; ++i) {
            if (tok[i] > 0xa0 || !strchr("0123456789-@", tok[i]))
                has_alpha = 1;
            if (tok[i] >= 'A' && tok[i] <= 'Z')
                term[i] = (unsigned char)((int)tok[i] + 'a' - 'A');
            else
                term[i] = tok[i];
        }

        if (!has_alpha)
            return;

        /* Update history. */
        memcpy(token_history[history_index].term, term, len);
        token_history[history_index].len = len;
        history_index = (history_index + 1) % HISTORY_LEN;
        if (ntokens_history < HISTORY_LEN)
            ++ntokens_history;

        /* Submit this token and composites with preceding ones. */
        record_tokens();
    }
}


