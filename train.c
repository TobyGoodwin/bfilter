/*

    Copyright (c) 2003 Chris Lightfoot. All rights reserved.
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

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "bfilter.h"
#include "class.h"
#include "db.h"
#include "db-count.h"
#include "db-term.h"
#include "error.h"
#include "read.h"
#include "skiplist.h"
#include "submit.h"
#include "train.h"
#include "util.h"

#define TRACE if (0)

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

static void init(void) {
    db_write();
    db_begin();
}

static void done(void) {
    db_count_done();
    db_term_done();
    db_commit();
    if (db_documents() % 100 == 0)
        db_vacuum();
    db_close();
}

const char *update_class = "\
UPDATE class \
  SET docs = docs + ?, terms = terms + ? \
  WHERE name = ?; \
";

/* Update database from our skiplist. */
void train_update(char *cclass, _Bool untrain) {
    int cid, tid;
    int signum = untrain ? -1 : 1;
    skiplist_iterator si;
    unsigned int nterms, ntermswr, ntermsnew, ntermsall;

    init();
    cid = class_id_furnish(cclass);

    TRACE fprintf(stderr, "cid is %d\n", cid);

    nterms = skiplist_size(token_list); /* distinct terms */
    ntermsall = 0; /* terms including dups */

    for (si = skiplist_itr_first(token_list), ntermswr = 0, ntermsnew = 0; si;
            si = skiplist_itr_next(token_list, si), ++ntermswr) {
        uint8_t *k;
        int n, *p;
        size_t kl;

        k = skiplist_itr_key(token_list, si, &kl);
        p = skiplist_itr_value(token_list, si);
        n = *p * signum; // copy, to avoid damaging token list for retrain case
        TRACE fprintf(stderr, "term %.*s: %d\n", (int)kl, k, n);
        tid = db_term_id_furnish(k, kl);
        TRACE fprintf(stderr, "tid is %d\n", tid);

        if (db_count_update(cid, tid, n))
            ++ntermsnew;
        ntermsall += n;

        if (isatty(1) && (ntermswr % 500) == 0)
            fprintf(stderr, "Writing: %u / %u terms (%u new)\r",
                    ntermswr, nterms, ntermsnew);
    }

    if (isatty(1))
        fprintf(stderr, "Writing: %u / %u terms (%u new)\n",
                ntermswr, nterms, ntermsnew);

    class_update(cid, nemails * signum, ntermsall);

    if (untrain) train_purge();
    done();
}

// call after untrain
void train_purge(void) {
    db_count_purge();
    db_term_purge();
}
