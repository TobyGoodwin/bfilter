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
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bayes.h"
#include "bfilter.h"
#include "class.h"
#include "db.h"
#include "db-term.h"
#include "error.h"
#include "settings.h"
#include "util.h"

#define TRACE if (1)

static int cmp(const void *x, const void *y) {
    const struct class *a = x;
    const struct class *b = y;
    const double c = a->logprob;
    const double d = b->logprob;
    if (c > d) return -1;
    if (c < d) return 1;
    return 0;
}

static const char q[] = "\
    SELECT class.id, COALESCE( \
        (SELECT count.count FROM count \
            WHERE class.id = count.class AND count.term = ?), \
        0) \
        FROM class \
";
static struct db_stmt sel = { q, sizeof q };

struct class *bayes(skiplist tokens, int *n) {
    int i, r;
    int n_class, docs, vocab;
    int *id2ix;
    skiplist_iterator si;
    struct class *classes;

    if (n) *n = 0;
    if (!db_read()) // missing database
        return 0;

    docs = db_documents();
    TRACE fprintf(stderr, "documents: %d\n", docs);
    vocab = db_vocabulary();
    TRACE fprintf(stderr, "vocabulary: %d\n", vocab);

    classes = class_fetch(&n_class, &id2ix);
    if (n) *n = n_class;
    TRACE fprintf(stderr, "classes: %d\n", n_class);

    /* establish priors */
    for (i = 0; i < n_class; ++i) {
        struct class *c = classes + i;
        TRACE fprintf(stderr, "class %s (%d)\n", c->name, c->id);
        TRACE fprintf(stderr, "emails in class %s: %d\n", c->name, c->docs);
        TRACE fprintf(stderr, "prior(%s): %f\n",
                c->name, (double)c->docs / docs);
        c->logprob = log((double)c->docs / (double)docs);
    }

    for (si = skiplist_itr_first(tokens); si;
            si = skiplist_itr_next(tokens, si)) {
        uint8_t *t;
        size_t t_len;
        int occurs; /* number of occurences of this term in test text */
        int tid = 0;

        t = skiplist_itr_key(tokens, si, &t_len);
        occurs = *(int *)skiplist_itr_value(tokens, si);

        if (!db_term_id_fetch(t, t_len, &tid))
            continue;

        r = db_stmt_ready(&sel);
        if (r != SQLITE_OK) db_fatal("prepare / reset", q);

	r = sqlite3_bind_int(sel.x, 1, tid);
        if (r != SQLITE_OK) db_fatal("bind first", q);

        // TRACE fprintf(stderr, "token %.*s\n", (int)t_len, t);

        while ((r = sqlite3_step(sel.x)) == SQLITE_ROW) {
            double p;
            int id, x;
            struct class *c;

            if (sqlite3_column_type(sel.x, 0) != SQLITE_INTEGER)
                fatal3("col 0 of `", q, "' has non-integer type");
            id = sqlite3_column_int(sel.x, 0);

            if (sqlite3_column_type(sel.x, 1) != SQLITE_INTEGER)
                fatal3("col 1 of `", q, "' has non-integer type");
            x = sqlite3_column_int(sel.x, 1);

            c = classes + id2ix[id];

            TRACE fprintf(stderr, "Tct = %d\n", x);
            p = (x + 1.) / (c->terms + vocab);
            TRACE fprintf(stderr, "condprob[%s][%.*s] = %g\n", c->name, (int)t_len, t, p);
            c->logprob += occurs * log(p);
            TRACE fprintf(stderr, "class prob: %g\n", c->logprob);
        }
    }

    db_stmt_finalize(&sel);

    qsort(classes, n_class, sizeof *classes, cmp);
    return classes;
}
