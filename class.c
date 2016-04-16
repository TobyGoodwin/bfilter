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

#include <arpa/inet.h>
#include <assert.h>
#include <sqlite3.h>
#include <string.h>

#include "bfilter.h"
#include "class.h"
#include "db.h"
#include "error.h"
#include "line.h"
#include "util.h"

static const char *get_classes = "\
SELECT id, name, docs, terms FROM class ORDER BY id; \
";

struct class *class_fetch(void) {
    int csa = 0, csn = 0, r;
    sqlite3 *db = db_db();
    sqlite3_stmt *stmt;
    struct class *cs = 0;

    r = sqlite3_prepare_v2(db, get_classes, strlen(get_classes), &stmt, 0);
    if (r != SQLITE_OK)
        fatal2("cannot prepare statement: ", sqlite3_errmsg(db));
    while ((r = sqlite3_step(stmt)) == SQLITE_ROW) {
        int i;

        if (csn == csa)
            cs = xrealloc(cs, (csa = csa * 2 + 1) * sizeof *cs);

        if (sqlite3_column_type(stmt, 0) != SQLITE_INTEGER)
            fatal1("class.id has non-integer type");
        cs[csn].code = sqlite3_column_int(stmt, 0);

        if (sqlite3_column_type(stmt, 1) != SQLITE_TEXT)
            fatal1("class.name has non-text type");
        cs[csn].name = sqlite3_column_text(stmt, 1);

        if (sqlite3_column_type(stmt, 2) != SQLITE_INTEGER)
            fatal1("class.docs has non-integer type");
        cs[csn].docs = sqlite3_column_int(stmt, 2);

        if (sqlite3_column_type(stmt, 3) != SQLITE_INTEGER)
            fatal1("class.terms has non-integer type");
        cs[csn].terms = sqlite3_column_int(stmt, 3);
    }
    sqlite3_finalize(stmt);

    /* add two sentinels */
    if (csn + 1 >= csa)
        cs = xrealloc(cs, (csa += 2) * sizeof *cs);

    cs[csn].name = 0;
    cs[csn].code = cs[csn].docs = cs[csn].terms = 0;

    ++csn;
    cs[csn].name = 0;
    cs[csn].code = cs[csn].docs = cs[csn].terms = 0;

    return cs;
}
#if 0
struct class *class_fetch(void) {
    size_t x_sz;
    uint8_t *p, *x;
    struct class *cs = 0;
    int csa = 0, csn = 0;

    x = db_hash_fetch((uint8_t *)KEY_CLASSES, sizeof(KEY_CLASSES) - 1, &x_sz);
    if (x) {
        /* note that the cs array we build here contains pointers into the data
         * x returned from the database; tdb specifies that the caller is
         * responsible for freeing this data, so obviously in this case we
         * don't free */
        for (p = x; p < x + x_sz; ++csn) {
            int i;
            uint32_t nc[3];
            if (csn == csa)
                cs = xrealloc(cs, (csa = csa * 2 + 1) * sizeof *cs);
            cs[csn].name = p;
            while (*p++)
                ;
            for (i = 0; i < 3; ++i) {
                memcpy(nc + i, p, sizeof *nc);
                p += sizeof *nc;
            }
            cs[csn].code = ntohl(nc[0]);
            cs[csn].docs = ntohl(nc[1]);
            cs[csn].terms = ntohl(nc[2]);
        }
    } else
        csa = csn = 0;
        
    /* add two sentinels */
    if (csn + 1 >= csa)
        cs = xrealloc(cs, (csa += 2) * sizeof *cs);

    cs[csn].name = 0;
    cs[csn].code = cs[csn].docs = cs[csn].terms = 0;

    ++csn;
    cs[csn].name = 0;
    cs[csn].code = cs[csn].docs = cs[csn].terms = 0;

    return cs;
}
#endif

#if 0
_Bool class_store(struct class *cs) {
    struct line csl = { 0 };
    struct class *p;

    for (p = cs; p->code; ++p) {
        int i;
        struct line c = { 0 };
        uint32_t nc[3];

        c.x = p->name;
        c.l = strlen((char *)p->name) + 1; /* including \0 */
        line_cat(&csl, &c);
        nc[0] = htonl(p->code);
        nc[1] = htonl(p->docs);
        nc[2] = htonl(p->terms);
        c.l = 4;
        for (i = 0; i < 3; ++i) {
            c.x = (uint8_t *)(nc + i);
            line_cat(&csl, &c);
        }
    }
    return db_hash_store(
            (uint8_t *)KEY_CLASSES, sizeof(KEY_CLASSES) - 1, csl.x, csl.l);
}
#endif

/* warning: overwrites the first sentinel inserted by class_fetch(): you can
 * only call class_lookup() once between class_fetch() and class_store() */
struct class *class_lookup(struct class *cs, char *c) {
    int m, n;
    struct class *cp;

    m = 0;
    assert(cs); /* get_classes must abort if it fails to read */
    for (n = 0, cp = cs; cp->code; ++n, ++cp) {
        if (strcmp(c, (char *)cp->name) == 0)
            return cp;
        if (cp->code > m) m = cp->code;
    }

    cp->name = (uint8_t *)c;
    cp->code = m + 1;
    return cp;
}
