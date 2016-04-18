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

static const char sql_id[] = "SELECT id FROM term WHERE term = ?";
static const char sql_insert[] = "INSERT INTO term (term) VALUES (?)";
static const char sql_update[] =
    "UPDATE term SET docs = docs + ?, terms = terms + ? WHERE id = ?";

static _Bool db_term_id_fetch(sqlite3 *db, uint8_t *t, int tl, int *x) {
    int r;
    sqlite3_stmt *s;

    r = sqlite3_prepare_v2(db, sql_id, sizeof sql_id, &s, 0);
    if (r != SQLITE_OK)
        fatal4("cannot prepare statement `", sql_id, "': ",
                sqlite3_errmsg(db));

    r = sqlite3_bind_text(s, 1, (char *)t, tl, 0);
    if (r != SQLITE_OK)
        fatal2("cannot bind value: ", sqlite3_errmsg(db));

    r = sqlite3_step(s);
    if (r == SQLITE_ROW) {
        if (sqlite3_column_type(s, 0) != SQLITE_INTEGER)
            fatal1("term.id has non-integer type");
        *x = sqlite3_column_int(s, 0);
        sqlite3_finalize(s);
        return 1;
    } else if (r == SQLITE_DONE) {
    } else
        fatal2("cannot step statement: ", sqlite3_errmsg(db));
    sqlite3_finalize(s);
    return 0;
}

static void db_term_insert(sqlite3 *db, uint8_t *t, int tl) {
    int r;
    sqlite3_stmt *s;

    r = sqlite3_prepare_v2(db, sql_insert, sizeof sql_insert, &s, 0);
    if (r != SQLITE_OK)
        fatal4("cannot prepare stmt `", sql_insert, "': ", sqlite3_errmsg(db));
    r = sqlite3_bind_text(s, 1, (char *)t, tl, 0);
    if (r != SQLITE_OK)
        fatal2("cannot bind value: ", sqlite3_errmsg(db));
    r = sqlite3_step(s);
    if (r != SQLITE_DONE)
        fatal4("cannot step stmt `", sql_insert, "': ", sqlite3_errmsg(db));
    sqlite3_finalize(s);
}

int db_term_id_furnish(uint8_t *t, int tl) {
    int x;
    sqlite3 *db = db_db();

    //db_begin();

    x = 0;
    if (db_term_id_fetch(db, t, tl, &x))
        goto done;

    db_term_insert(db, t, tl);

    if (!db_term_id_fetch(db, t, tl, &x))
        fatal2("failed to insert term: ", sqlite3_errmsg(db));

done:
    //db_commit();
    return x;
}

void db_term_update(int cid, int nd, int nt) {
    int r;
    sqlite3 *db = db_db();
    sqlite3_stmt *s;

    r = sqlite3_prepare_v2(db, sql_update, sizeof sql_update, &s, 0);
    if (r != SQLITE_OK)
        fatal4("cannot prepare stmt `", sql_update, "': ", sqlite3_errmsg(db));

    r = sqlite3_bind_int(s, 1, nd);
    if (r != SQLITE_OK)
        fatal2("cannot bind first value: ", sqlite3_errmsg(db));

    r = sqlite3_bind_int(s, 2, nt);
    if (r != SQLITE_OK)
        fatal2("cannot bind second value: ", sqlite3_errmsg(db));

    r = sqlite3_bind_int(s, 3, cid);
    if (r != SQLITE_OK)
        fatal2("cannot bind third value: ", sqlite3_errmsg(db));

    r = sqlite3_step(s);
    if (r != SQLITE_DONE)
        fatal4("cannot step stmt `", sql_update, "': ", sqlite3_errmsg(db));
    sqlite3_finalize(s);
}
