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
        if (!read_email(flagb, 0, stdin, NULL)) {
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
    int nspam, nreal;
    char *term = NULL;
    size_t termlen = 1;
    skiplist_iterator si;
    unsigned int nterms, ntermswr, ntermsnew;

    if (!db_get_pair("__emails__", &nspam, &nreal))
        nspam = nreal = 0;
    if (mode == isspam)
        nspam += nemails;
    else
        nreal += nemails;
    db_set_pair("__emails__", nspam, nreal);

    if (isatty(1))
        fprintf(stderr, "Writing: corpus now contains %8u spam / %8u nonspam emails\n", nspam, nreal);

    nterms = skiplist_size(token_list);

    for (si = skiplist_itr_first(token_list), ntermswr = 0, ntermsnew = 0; si; si = skiplist_itr_next(token_list, si), ++ntermswr) {
        char *k;
        size_t kl;
        struct wordcount *pw;

        k = skiplist_itr_key(token_list, si, &kl);

        /* NUL-terminate */
        if (!term || kl + 1 > termlen)
            term = xrealloc(term, termlen = 2 * (kl + 1));
        term[kl] = 0;
        memcpy(term, k, kl);

        pw = skiplist_itr_value(token_list, si);
//fprintf(stderr, "%d %s \n", pw->n, term);

        if (!db_get_pair(term, &nspam, &nreal)) {
            nspam = nreal = 0;
            ++ntermsnew;
        }

        if (mode == isspam)
            nspam += pw->n;
        else
            nreal += pw->n;

        db_set_pair(term, nspam, nreal);

        if (isatty(1) && (ntermswr % 500) == 0)
            fprintf(stderr, "Writing: %8u / %8u terms (%8u new)\r", ntermswr, nterms, ntermsnew);
    }

    if (isatty(1))
        fprintf(stderr, "Writing: %8u / %8u terms (%8u new)\n", ntermswr, nterms, ntermsnew);

    free(term);
}
