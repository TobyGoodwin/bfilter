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

#define EMAILS_KEY ((uint8_t *)"__emails__")

void train_update(enum mode mode) {
    /* Update total number of emails and the data for each word. */
    uint32_t nemails, *pnemails;
    int mustbe1;
    skiplist_iterator si;
    unsigned int nterms, ntermswr, ntermsnew;

    pnemails = db_get_intlist(EMAILS_KEY, sizeof EMAILS_KEY - 1, &mustbe1);
    if (pnemails && mustbe1 == 1)
        nemails = *pnemails;
    else
        nemails = 0;
    ++nemails;
    db_set_intlist(EMAILS_KEY, sizeof EMAILS_KEY - 1, &nemails, 1);

    if (isatty(1))
        fprintf(stderr, "Writing: corpus now contains %u emails\n", nemails);

    nterms = skiplist_size(token_list);

    for (si = skiplist_itr_first(token_list), ntermswr = 0, ntermsnew = 0; si;
            si = skiplist_itr_next(token_list, si), ++ntermswr) {
        uint8_t *k;
        int *p;
        size_t kl;

        k = skiplist_itr_key(token_list, si, &kl);
        p = skiplist_itr_value(token_list, si);
        if (count_add(k, kl, tclass_c, *p))
            ++ntermsnew;

        /*
        if (!db_get_pair(term, &nspam, &nreal)) {
            nspam = nreal = 0;
            ++ntermsnew;
        }
        */

        /*
        if (mode == isspam)
            nspam += pw->n;
        else
            nreal += pw->n;

        db_set_pair(term, nspam, nreal);
        */

        if (isatty(1) && (ntermswr % 500) == 0)
            fprintf(stderr, "Writing: %u / %u terms (%u new)\r",
                    ntermswr, nterms, ntermsnew);
    }

    if (isatty(1))
        fprintf(stderr, "Writing: %u / %u terms (%u new)\n",
                ntermswr, nterms, ntermsnew);

}
