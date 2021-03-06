/*

    Copyright (c) 2003 - 2004 Chris Lightfoot. All rights reserved.
    Copyright (c) 2015 - 2016 Toby Goodwin.
    toby@paccrat.org
    https://github.com/TobyGoodwin/bfilter

    This file is part of bfilter.

    Bfilter is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Bfilter is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with bfilter.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "compose.h"
#include "settings.h"
#include "skiplist.h"
#include "submit.h"

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

