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
#include "db.h"
#include "db-term.h"
#include "error.h"

static const char sql_update[] =
    "UPDATE term SET docs = docs + ?, terms = terms + ? WHERE id = ?";

static const char sql_fetch[] = "SELECT id FROM term WHERE term = ?";
struct db_stmt fetch = { sql_fetch, sizeof sql_fetch };
static void fetch_done(void) { db_stmt_finalize(&fetch); };

_Bool db_term_id_fetch(uint8_t *t, int tl, int *x) {
    sqlite3 *db = db_db();
    int r;

    if (fetch.x)
        r = sqlite3_reset(fetch.x);
    else
        r = sqlite3_prepare_v2(db, fetch.s, fetch.n, &fetch.x, 0);

    if (r != SQLITE_OK) db_fatal("prepare / reset", fetch.s);

    r = sqlite3_bind_text(fetch.x, 1, (char *)t, tl, 0);
    if (r != SQLITE_OK) db_fatal("bind first", fetch.s);

    r = sqlite3_step(fetch.x);
    switch (r) {
        case SQLITE_ROW:
            if (sqlite3_column_type(fetch.x, 0) != SQLITE_INTEGER)
                fatal1("term.id has non-integer type");
            *x = sqlite3_column_int(fetch.x, 0);
            return 1;

        case SQLITE_DONE:
            break;

        default:
            db_fatal("step", fetch.s);
    }

    return 0;
}

static const char sql_insert[] = "INSERT INTO term (term) VALUES (?)";

struct db_stmt insert = { sql_insert, sizeof sql_insert };
void insert_done(void) { db_stmt_finalize(&insert); }

static void db_term_insert(sqlite3 *db, uint8_t *t, int tl) {
    int r;

    if (insert.x)
        r = sqlite3_reset(insert.x);
    else
        r = sqlite3_prepare_v2(db, insert.s, insert.n, &insert.x, 0);

    if (r != SQLITE_OK) db_fatal("prepare / reset", insert.s);

    r = sqlite3_bind_text(insert.x, 1, (char *)t, tl, 0);
    if (r != SQLITE_OK) db_fatal("bind first", insert.s);

    r = sqlite3_step(insert.x);
    if (r != SQLITE_DONE) db_fatal("step", insert.s);
}

// needs to run inside a transaction, so that the insert cannot fail (terms are
// unique)
int db_term_id_furnish(uint8_t *t, int tl) {
    int x;
    sqlite3 *db = db_db();

    x = 0;
    if (db_term_id_fetch(t, tl, &x))
        return x;

    db_term_insert(db, t, tl);

    if (!db_term_id_fetch(t, tl, &x))
        fatal2("failed to insert term: ", sqlite3_errmsg(db));

    return x;
}

void db_term_update(int cid, int nd, int nt) {
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

void db_term_done(void) {
    fetch_done();
    insert_done();
}
