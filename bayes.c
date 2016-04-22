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

#include <assert.h>
#include <errno.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bayes.h"
#include "bfilter.h"
#include "count.h"
#include "db.h"
#include "error.h"
#include "settings.h"
#include "util.h"

#define TRACE if (1)

static int cmp(const void *x, const void *y) {
    const struct bayes_result *a = x;
    const struct bayes_result *b = y;
    const double c = a->logprob;
    const double d = b->logprob;
    if (c > d) return -1;
    if (c < d) return 1;
    return 0;
}

static struct bayes_result *sort(struct bayes_result *x, int n) {
    qsort(x, n, sizeof *x, cmp);
    return x;
}


struct bayes_result *bayes(skiplist tokens, int *n) {
    static const char q[] = "SELECT id, name, docs, terms FROM class";
    int r;
    int docs, vocab, classes;
    sqlite3 *db = db_db();
    sqlite3_stmt *stmt;
    struct bayes_result *result;

    classes = db_classes();
    if (n) *n = classes;
    TRACE fprintf(stderr, "classes: %d\n", classes);
    docs = db_documents();
    TRACE fprintf(stderr, "documents: %d\n", docs);
    vocab = db_vocabulary();
    TRACE fprintf(stderr, "vocabulary: %d\n", vocab);

    result = xmalloc(classes * sizeof *result);

    r = sqlite3_prepare_v2(db, q, sizeof q, &stmt, 0);
    if (r != SQLITE_OK)
        fatal4("cannot prepare statement `", q, "': ", sqlite3_errmsg(db));
    while ((r = sqlite3_step(stmt)) == SQLITE_ROW) {
        const uint8_t *c_name;
        int c_id, c_docs, c_terms;

        assert(sqlite3_column_type(stmt, 0) == SQLITE_INTEGER);
        assert(sqlite3_column_type(stmt, 1) == SQLITE_TEXT);
        assert(sqlite3_column_type(stmt, 2) == SQLITE_INTEGER);
        assert(sqlite3_column_type(stmt, 3) == SQLITE_INTEGER);

        c_id = sqlite3_column_int(stmt, 0);
        c_name = sqlite3_column_text(stmt, 1);
        c_docs = sqlite3_column_int(stmt, 2);
        c_terms = sqlite3_column_int(stmt, 3);

        fprintf(stderr, "%d %s %d %d\n", c_id, c_name, c_docs, c_terms);
    }
    if (r != SQLITE_DONE)
        fatal4("cannot step statement `", q, "': ", sqlite3_errmsg(db));
    sqlite3_finalize(stmt);

    return 0;
#if 0
    struct class *class, *classes;
    int c, i, n_class = 0, n_total;
    struct bayes_result *r;
    uint32_t *p_ui32, t_total;
   
    if (n) *n = 0;

    //classes = class_fetch();
    assert(classes);
    if (classes->code == 0)
        return 0;

    p_ui32 = db_hash_fetch_uint32((uint8_t *)KEY_DOCUMENTS,
            sizeof KEY_DOCUMENTS - 1);
    if (p_ui32) n_total = *p_ui32;
    else return 0;

    p_ui32 = db_hash_fetch_uint32((uint8_t *)KEY_VOCABULARY,
            sizeof KEY_VOCABULARY - 1);
    if (p_ui32) t_total = *p_ui32;
    else return 0;

    TRACE fprintf(stderr, "documents (emails trained): %d\n", n_total);
    TRACE fprintf(stderr, "vocabulary (total distinct terms): %d\n", t_total);

    // how many classes do we have?
    for (class = classes; class->code; ++class)
        ++n_class;

    if (n) *n = n_class;
    r = xmalloc(n_class * sizeof *r);

    for (c = 0, class = classes; class->code; ++c, ++class) {
        double lp;
        skiplist_iterator si;

        TRACE fprintf(stderr, "class %s (%d)\n", class->name, class->code);
        TRACE fprintf(stderr, "emails in class %s: %d\n",
                class->name, class->docs);
        TRACE fprintf(stderr, "prior(%s): %f\n",
                class->name, (double)class->docs / n_total);
        r[c].category = class->name;
        lp = log((double)class->docs / (double)n_total);

        TRACE fprintf(stderr, "t_%s = %d, t_total = %d\n",
                class->name, class->terms, t_total);
        TRACE fprintf(stderr, "terms in class %s: %d\n",
                class->name, class->terms);
        for (si = skiplist_itr_first(tokens); si;
                si = skiplist_itr_next(tokens, si)) {
            double p;
            uint8_t *t;
            size_t t_len;
            uint32_t *cnts;
            unsigned int ncnts, Tct;
            int occurs; /* number of occurences of this term in test text */

            t = skiplist_itr_key(tokens, si, &t_len);
            occurs = *(int *)skiplist_itr_value(tokens, si);
            cnts = db_get_intlist(t, t_len, &ncnts);
            if (!cnts) continue; /* not in training vocabulary */
            Tct = 0;
            for (i = 0; i < ncnts; i += 2)
                if (cnts[i] == class->code) {
                    Tct = cnts[i + 1];
                    break;
                }
            TRACE fprintf(stderr, "Tct = %d\n", Tct);
            p = (Tct + 1.) / (class->terms + t_total);
            TRACE fprintf(stderr, "condprob[%s][%.*s] = %g\n",
                    class->name, (int)t_len, t, p);
            lp += occurs * log(p);
        }
        r[c].logprob = lp;
    }

    sort(r, n_class);
    return r;
#endif
}
