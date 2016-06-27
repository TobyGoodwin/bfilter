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
static struct db_stmt exists = { sql_exists, sizeof sql_exists };

static _Bool db_count_exists(int c, int t) {
    int r;

    r = db_stmt_ready(&exists);
    if (r != SQLITE_OK) db_fatal("prepare / reset", exists.s);

    r = sqlite3_bind_int(exists.x, 1, c);
    if (r != SQLITE_OK) db_fatal("bind first", exists.s);

    r = sqlite3_bind_int(exists.x, 2, t);
    if (r != SQLITE_OK) db_fatal("bind second", exists.s);

    r = sqlite3_step(exists.x);
    switch (r) {
        case SQLITE_ROW:
            return 1;
        case SQLITE_DONE:
            return 0;
        default:
            db_fatal("step", exists.s);
    }
    return 0;
}

static const char sql_insert[] =
    "INSERT INTO count (class, term, count) VALUES (?, ?, 0)";
static struct db_stmt insert = { sql_insert, sizeof sql_insert };

static void db_count_insert(int c, int t) {
    int r;

    r = db_stmt_ready(&insert);
    if (r != SQLITE_OK) db_fatal("prepare / reset", insert.s);

    r = sqlite3_bind_int(insert.x, 1, c);
    if (r != SQLITE_OK) db_fatal("bind first", insert.s);

    r = sqlite3_bind_int(insert.x, 2, t);
    if (r != SQLITE_OK) db_fatal("bind second", insert.s);

    r = sqlite3_step(insert.x);
    if (r != SQLITE_DONE) db_fatal("step", insert.s);
}

// needs to run inside a transaction, so insert cannot fail
static _Bool db_count_furnish(int c, int t) {
    if (db_count_exists(c, t))
        return 0;

    db_count_insert(c, t);
    return 1;
}

static const char sql_update[] =
    "UPDATE count SET count = count + ? WHERE class = ? AND term = ?";
struct db_stmt update = { sql_update, sizeof sql_update };

_Bool db_count_update(int c, int t, int n) {
    _Bool x;
    int r;

    r = db_stmt_ready(&update);
    if (r != SQLITE_OK) db_fatal("prepare / reset", update.s);

    x = db_count_furnish(c, t);

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

static const char sql_purge[] = "DELETE FROM count WHERE count <= 0";
static struct db_stmt purge = { sql_purge, sizeof sql_purge };

void db_count_purge(void) {
    int r;

    r = db_stmt_ready(&purge);
    if (r != SQLITE_OK) db_fatal("prepare / reset", purge.s);

    r = sqlite3_step(purge.x);
    if (r != SQLITE_DONE) db_fatal("step", purge.s);

    db_stmt_finalize(&purge);
}

void db_count_done(void) {
    db_stmt_finalize(&exists);
    db_stmt_finalize(&insert);
    db_stmt_finalize(&update);
}
