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
#include "db-count.h"
#include "error.h"

static const char sql_exists[] =
    "SELECT 0 FROM count WHERE class = ? AND term = ?";
static const char sql_insert[] =
    "INSERT INTO count (class, term, count) VALUES (?, ?, 0)";

static _Bool db_count_exists(sqlite3 *db, int c, int t) {
    int r;
    sqlite3_stmt *s;

    r = sqlite3_prepare_v2(db, sql_exists, sizeof sql_exists, &s, 0);
    if (r != SQLITE_OK) db_fatal("prepare", sql_exists);

    r = sqlite3_bind_int(s, 1, c);
    if (r != SQLITE_OK) db_fatal("bind first", sql_exists);

    r = sqlite3_bind_int(s, 2, t);
    if (r != SQLITE_OK) db_fatal("bind second", sql_exists);

    r = sqlite3_step(s);
    if (r == SQLITE_ROW) {
        sqlite3_finalize(s);
        return 1;
    } else if (r == SQLITE_DONE) {
    } else
        db_fatal("step", sql_exists);

    sqlite3_finalize(s);
    return 0;
}

static void db_count_insert(sqlite3 *db, int c, int t) {
    int r;
    sqlite3_stmt *s;

    r = sqlite3_prepare_v2(db, sql_insert, sizeof sql_insert, &s, 0);
    if (r != SQLITE_OK) db_fatal("prepare", sql_insert);

    r = sqlite3_bind_int(s, 1, c);
    if (r != SQLITE_OK) db_fatal("bind first", sql_insert);

    r = sqlite3_bind_int(s, 2, t);
    if (r != SQLITE_OK) db_fatal("bind second", sql_insert);

    r = sqlite3_step(s);
    if (r != SQLITE_DONE) db_fatal("step", sql_insert);

    sqlite3_finalize(s);
}

// needs to run inside a transaction, so insert cannot fail
static _Bool db_count_furnish(sqlite3 *db, int c, int t) {
    if (db_count_exists(db, c, t))
        return 0;

    db_count_insert(db, c, t);
    return 1;
}

static const char sql_update[] =
    "UPDATE count SET count = count + ? WHERE class = ? AND term = ?";
struct db_stmt update = { sql_update, sizeof sql_update };
void db_count_done(void) { db_stmt_finalize(&update); }

_Bool db_count_update(int c, int t, int n) {
    _Bool x;
    int r;
    sqlite3 *db = db_db();

    r = db_stmt_ready(&update);
    if (r != SQLITE_OK) db_fatal("prepare / reset", update.s);

    x = db_count_furnish(db, c, t);

    r = sqlite3_bind_int(update.x, 1, n);
    if (r != SQLITE_OK) db_fatal("bind first", update.s);

    r = sqlite3_bind_int(update.x, 2, c);
    if (r != SQLITE_OK) db_fatal("bind second", update.s);

    r = sqlite3_bind_int(update.x, 3, t);
    if (r != SQLITE_OK) db_fatal("bind third", update.s);

    r = sqlite3_step(update.x);
    if (r != SQLITE_DONE) db_fatal("step", update.s);

    return x;
}
