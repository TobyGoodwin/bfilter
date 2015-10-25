/*

    Copyright (c) 2003 Chris Lightfoot. All rights reserved.
    Copyright (c) 2015 Toby Goodwin.
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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "bfilter.h"
#include "count.h"
#include "db.h"
#include "read.h"
#include "skiplist.h"
#include "submit.h"
#include "train.h"
#include "util.h"

_Bool train_read(void) {
    do {
        errno = 0;
        ++nemails;
        if (!read_email(flagb, stdin, 0)) {
            fprintf(stderr, "bfilter: error while reading email (%s)\n",
                    errno ? strerror(errno) : "no system error");
            return 0;
        }

        /* If we're running on a terminal, print stats. */
        if (isatty(1))
            fprintf(stderr,
        "Reading: %8u emails (%8u bytes) %8lu terms avg length %8.2f\r",
                nemails, (unsigned)nbytesrd, skiplist_size(token_list),
                (double)term_length / skiplist_size(token_list));
    } while (!feof(stdin));
    if (isatty(1))
        fprintf(stderr, "\n");
    return 1;
}

void train_update(enum mode mode) {
    /* Update total number of emails and the data for each word. */
    uint32_t Ndb, *pNdb;
    uint32_t nvocab, *pnvocab;
    int mustbe1;
    skiplist_iterator si;
    unsigned int nterms, ntermswr, ntermsnew, ntermsall;

    pNdb = db_get_intlist((uint8_t *)EMAILS_KEY,
            sizeof EMAILS_KEY - 1, &mustbe1);
    if (pNdb && mustbe1 == 1)
        Ndb = *pNdb;
    else
        Ndb = 0;
    Ndb += nemails;
    db_set_intlist((uint8_t *)EMAILS_KEY, sizeof EMAILS_KEY - 1, &Ndb, 1);

    count_add((uint8_t *)EMAILS_CLASS_KEY, sizeof EMAILS_CLASS_KEY - 1,
            tclass_c, nemails);

    if (isatty(1))
        fprintf(stderr, "Writing: corpus now contains %u emails\n", Ndb);

    nterms = skiplist_size(token_list); /* distinct terms */
    ntermsall = 0; /* terms including dups */

    for (si = skiplist_itr_first(token_list), ntermswr = 0, ntermsnew = 0; si;
            si = skiplist_itr_next(token_list, si), ++ntermswr) {
        uint8_t *k;
        int *p;
        size_t kl;

        k = skiplist_itr_key(token_list, si, &kl);
        p = skiplist_itr_value(token_list, si);
if (0) fprintf(stderr, "term %.*s: %d\n", (int)kl, k, *p);
        if (count_add(k, kl, tclass_c, *p))
            ++ntermsnew;
        ntermsall += *p;

        if (isatty(1) && (ntermswr % 500) == 0)
            fprintf(stderr, "Writing: %u / %u terms (%u new)\r",
                    ntermswr, nterms, ntermsnew);
    }

    if (isatty(1))
        fprintf(stderr, "Writing: %u / %u terms (%u new)\n",
                ntermswr, nterms, ntermsnew);
    
    pnvocab = db_get_intlist((uint8_t *)VOCAB_KEY,
            sizeof VOCAB_KEY - 1, &mustbe1);
    if (pnvocab && mustbe1 == 1)
        nvocab = *pnvocab;
    else
        nvocab = 0;
    nvocab += ntermsnew;
    db_set_intlist((uint8_t *)VOCAB_KEY, sizeof VOCAB_KEY - 1, &nvocab, 1);

    count_add((uint8_t *)TERMS_CLASS_KEY, sizeof TERMS_CLASS_KEY - 1,
            tclass_c, ntermsall);

}
