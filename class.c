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

#include <sqlite3.h>
#include <string.h>

#include "bfilter.h"
#include "class.h"
#include "db.h"
#include "error.h"
#include "util.h"

static const char sql_cid[] = "SELECT id FROM class WHERE name = ?";
static const char sql_insert[] =
    "INSERT INTO class (name, docs, terms) VALUES (?, 0, 0)";
static const char sql_update[] =
    "UPDATE class SET docs = docs + ?, terms = terms + ? WHERE id = ?";

static _Bool db_class_id_fetch(sqlite3 *db, char *c, int *x) {
    int r;
    sqlite3_stmt *s;

    r = sqlite3_prepare_v2(db, sql_cid, sizeof sql_cid, &s, 0);
    if (r != SQLITE_OK) db_fatal("prepare", sql_cid);

    r = sqlite3_bind_text(s, 1, c, strlen(c), 0);
    if (r != SQLITE_OK) db_fatal("bind first", sql_cid);

    r = sqlite3_step(s);
    switch (r) {
        case SQLITE_ROW:
            if (sqlite3_column_type(s, 0) != SQLITE_INTEGER)
                fatal1("class.name has non-integer type");
            *x = sqlite3_column_int(s, 0);
            sqlite3_finalize(s);
            return 1;

        case SQLITE_DONE:
            break;

        default:
            db_fatal("step", sql_cid);
    }

    sqlite3_finalize(s);
    return 0;
}

static void db_class_insert(sqlite3 *db, char *c) {
    int r;
    sqlite3_stmt *s;

    r = sqlite3_prepare_v2(db, sql_insert, sizeof sql_insert, &s, 0);
    if (r != SQLITE_OK) db_fatal("prepare", sql_insert);

    r = sqlite3_bind_text(s, 1, c, strlen(c), 0);
    if (r != SQLITE_OK) db_fatal("bind first", sql_insert);

    r = sqlite3_step(s);
    if (r != SQLITE_DONE) db_fatal("step", sql_insert);

    sqlite3_finalize(s);
}

// needs to run inside a transaction
int class_id_furnish(char *c) {
    int x;
    sqlite3 *db = db_db();

    x = 0;
    if (db_class_id_fetch(db, c, &x))
        return x;

    db_class_insert(db, c);

    if (!db_class_id_fetch(db, c, &x))
        fatal4("failed to insert class `", c, "': ", sqlite3_errmsg(db));

    return x;
}

void class_update(int cid, int nd, int nt) {
    int r;
    sqlite3 *db = db_db();
    sqlite3_stmt *s;

    r = sqlite3_prepare_v2(db, sql_update, sizeof sql_update, &s, 0);
    if (r != SQLITE_OK) db_fatal("prepare", sql_update);

    r = sqlite3_bind_int(s, 1, nd);
    if (r != SQLITE_OK) db_fatal("bind first", sql_update);

    r = sqlite3_bind_int(s, 2, nt);
    if (r != SQLITE_OK) db_fatal("bind second", sql_update);

    r = sqlite3_bind_int(s, 3, cid);
    if (r != SQLITE_OK) db_fatal("bind third", sql_update);

    r = sqlite3_step(s);
    if (r != SQLITE_DONE) db_fatal("step", sql_update);

    sqlite3_finalize(s);
}

static int class_count(void) {
    static const char q[] = "SELECT COUNT(1) FROM class";
    return db_int_query(q, sizeof q);
}

static int class_max(void) {
    static const char q[] = "SELECT MAX(id) FROM class";
    return db_int_query(q, sizeof q);
}

struct class *class_fetch(int *n, int **id2ix) {
    int i, max, num, r;
    struct class *cs = 0;
    static const char sql[] =
        "SELECT id, name, docs, terms FROM class";
    static struct db_stmt q = { sql, sizeof sql }; 

    db_begin(); // run inside transaction to ensure allocations big enough
    num = class_count(); // number of classes
    max = class_max() + 1; // biggest class id for id2ix map

    cs = xmalloc(num * sizeof *cs);
    if (n) *n = num;
    if (id2ix) *id2ix = xmalloc(max * sizeof **id2ix);

    r = db_stmt_ready(&q);
    if (r != SQLITE_OK) db_fatal("prepare", q.s);

    for (i = 0; (r = sqlite3_step(q.x)) == SQLITE_ROW; ++i) {
        struct class *cp = cs + i;

        if (sqlite3_column_type(q.x, 0) != SQLITE_INTEGER)
            fatal1("class.id has non-integer type");
        cp->id = sqlite3_column_int(q.x, 0);
        if (id2ix) (*id2ix)[cp->id] = i;

        if (sqlite3_column_type(q.x, 1) != SQLITE_TEXT)
            fatal1("class.name has non-text type");
        cp->name = u8_xstrdup(sqlite3_column_text(q.x, 1));

        if (sqlite3_column_type(q.x, 2) != SQLITE_INTEGER)
            fatal1("class.docs has non-integer type");
        cp->docs = sqlite3_column_int(q.x, 2);

        if (sqlite3_column_type(q.x, 3) != SQLITE_INTEGER)
            fatal1("class.terms has non-integer type");
        cp->terms = sqlite3_column_int(q.x, 3);

        cp->logprob = 0.0;
    }

    if (r != SQLITE_DONE) db_fatal("step", q.s);

    db_stmt_finalize(&q);
    db_commit();

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

#if 0
/* warning: overwrites the first sentinel inserted by class_fetch(): you can
 * only call class_lookup() once between class_fetch() and class_store() */
struct class *class_lookup(struct class *cs, char *c) {
    int m, n;
    struct class *cp;

    m = 0;
    assert(cs); /* get_classes must abort if it fails to read */
    for (n = 0, cp = cs; cp->id; ++n, ++cp) {
        if (strcmp(c, (char *)cp->name) == 0)
            return cp;
        if (cp->id > m) m = cp->id;
    }

    cp->name = (uint8_t *)c;
    cp->id = m + 1;
    return cp;
}
#endif
