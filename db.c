/*
 * db.c:
 * Database for bfilter.
 *
 * Copyright (c) 2004 Chris Lightfoot. All rights reserved.
 * Email: chris@ex-parrot.com; WWW: http://www.ex-parrot.com/~chris/
 *
 */

#include <sys/types.h>

#include <errno.h>
#include <pwd.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <tdb.h>
#include <time.h>
#include <unistd.h>

#include <sqlite3.h>

#include "bfilter.h"
#include "db.h"
#include "error.h"
#include "settings.h"
#include "util.h"

static sqlite3 *db = 0;

struct sqlite3 *db_db(void) {
        return db;
}

void db_fatal(const char *verb, const char *query) {
    fatal6("failed to ", verb, " `", query, "': ", sqlite3_errmsg(db));
}

static char *dbfilename(const char *suffix) {
    char *name, *home;
    if ((home = getenv("BFILTER_DB")))
        name = xstrdup(home);
    else {
        home = getenv("HOME");
        if (!home) {
            struct passwd *P;
            if (!(P = getpwuid(getuid())))
                return 0;
            home = P->pw_dir;
        }
        name = xmalloc(strlen(home) + strlen(DATABASE_NAME) + 2);
        sprintf(name, "%s/%s", home, DATABASE_NAME);
    }
    if (suffix) {
        name = xrealloc(name, strlen(name) + strlen(suffix) + 1);
        strcat(name, suffix);
    }
    return name;
}

static const char create[] = "\
CREATE TABLE class ( \
  id INTEGER PRIMARY KEY, \
  name TEXT NOT NULL, \
  docs INTEGER NOT NULL, \
  terms INTEGER NOT NULL, \
   UNIQUE (name) ); \
CREATE TABLE term ( \
  id INTEGER PRIMARY KEY, \
  term TEXT NOT NULL UNIQUE ); \
CREATE TABLE count ( \
  class INTEGER NOT NULL, \
  term INTEGER NOT NULL, \
  count INTEGER, \
    PRIMARY KEY (class, term), \
    FOREIGN KEY (class) REFERENCES class (id), \
    FOREIGN KEY (term) REFERENCES term (id)); \
CREATE TABLE version ( \
  version INTEGER NOT NULL ); \
";

static const char set_version[] = "INSERT INTO version (version) VALUES (?)";

void db_init(void) {
    char *errmsg = 0;
    int r;
    sqlite3_stmt *stmt;

    sqlite3_exec(db, create, 0, 0, &errmsg);
    if (errmsg != 0)
        fatal2("cannot initialize database: ", errmsg);

    r = sqlite3_prepare_v2(db, set_version, sizeof set_version, &stmt, 0);
    if (r != SQLITE_OK) db_fatal("prepare", set_version);

    r = sqlite3_bind_int(stmt, 1, VERSION);
    if (r != SQLITE_OK) db_fatal("bind", set_version);

    r = sqlite3_step(stmt);
    if (r != SQLITE_DONE) db_fatal("step", set_version);

    sqlite3_finalize(stmt);
}

// Perform a query that should return a single integer. If the result is NULL
// (e.g. "SELECT count(1) FROM ..." on an empty table) we return 0. Any other
// problem is a fatal error. Does not check that only a single row is returned.
int db_int_query(const char *q, size_t qn) {
    int r, x;
    sqlite3_stmt *stmt;

    r = sqlite3_prepare_v2(db, q, qn, &stmt, 0);
    if (r != SQLITE_OK) db_fatal("prepare", q);

    r = sqlite3_step(stmt);
    if (r != SQLITE_ROW) db_fatal("step", q);
    switch (sqlite3_column_type(stmt, 0)) {
        case SQLITE_NULL:
            x = 0;
            break;

        case SQLITE_INTEGER:
            x = sqlite3_column_int(stmt, 0);
            break;

        default:
            fatal3("result of `", q, "' has non-integer type");
            x = 0; // silence compiler warning
            break;
    }

    sqlite3_finalize(stmt);
    return x;
}

int db_stmt_ready(struct db_stmt *s) {
    if (s->x)
        return sqlite3_reset(s->x);
    else
        return sqlite3_prepare_v2(db, s->s, s->n, &s->x, 0);
}

void db_stmt_finalize(struct db_stmt *s) {
    if (s->x) {
        sqlite3_finalize(s->x);
        s->x = 0;
    }
}

void db_check_version(void) {
    static const char q[] = "SELECT version FROM version";
    int v;

    v = db_int_query(q, sizeof q);
    if (v < MIN_VERSION || v > VERSION) {
        char vs[25];
        snprintf(vs, 25, "%d", v);
        fatal2("bad database version: ", vs);
    }
}

// open the database, read-only or read/write as specified by the flag. if the
// database is missing: create in write mode, return 0 in read mode.
static int db_open(_Bool write) {
    char *name;
    int err, flags;

    name = dbfilename(NULL);

    if (write)
        flags = SQLITE_OPEN_READWRITE;
    else
        flags = SQLITE_OPEN_READONLY;

    err = sqlite3_open_v2(name, &db, flags, 0);

    if (err != SQLITE_OK && errno == ENOENT) {
        if (!write) return 0;
        flags |= SQLITE_OPEN_CREATE;
        err = sqlite3_open_v2(name, &db, flags, 0);
        if (err == SQLITE_OK)
            db_init();
    }

    if (err != SQLITE_OK) {
        fatal4("cannot open database `", name, "': ", sqlite3_errmsg(db));
        sqlite3_close(db);
    }

    sqlite3_extended_result_codes(db, 1);

    db_check_version();
    return 1;
}

int db_read(void) { return db_open(0); }
void db_write(void) { db_open(1); }

/* db_close
 * Close the filter database. */
void db_close(void) {
    sqlite3_close_v2(db);
}

static const char sql_begin[] = "BEGIN TRANSACTION";

void db_begin(void) {
    char *errmsg;

    sqlite3_exec(db, sql_begin, 0, 0, &errmsg);
    if (errmsg) fatal2("cannot begin transaction: ", errmsg);
}

static const char sql_commit[] = "COMMIT";

void db_commit(void) {
    char *errmsg;

    sqlite3_exec(db, sql_commit, 0, 0, &errmsg);
    if (errmsg) fatal2("cannot commit transaction: ", errmsg);
}

static const char sql_vacuum[] = "VACUUM";

void db_vacuum(void) {
    char *errmsg;

    sqlite3_exec(db, sql_vacuum, 0, 0, &errmsg);
    if (errmsg) fatal2("cannot vacuum: ", errmsg);
}

#if 0
int db_classes(void) {
    static const char q[] = "SELECT COUNT(1) FROM class";
    return db_int_query(q, sizeof q);
}
#endif

int db_documents(void) {
    static const char q[] = "SELECT SUM(docs) FROM class";
    return db_int_query(q, sizeof q);
}

int db_vocabulary(void) {
    static const char q[] = "SELECT COUNT(1) FROM term";
    return db_int_query(q, sizeof q);
}

void db_class_rename(const char *old, const char *new) {
    int r;
    sqlite3_stmt* stmt;
    static const char q[] = "UPDATE class SET name = ? WHERE name = ?";

    db_write();

    r = sqlite3_prepare_v2(db, q, sizeof q, &stmt, 0);
    if (r != SQLITE_OK) db_fatal("prepare", q);

    r = sqlite3_bind_text(stmt, 1, new, strlen(new), 0);
    if (r != SQLITE_OK) db_fatal("bind 1", q);

    r = sqlite3_bind_text(stmt, 2, old, strlen(old), 0);
    if (r != SQLITE_OK) db_fatal("bind 2", q);

    r = sqlite3_step(stmt);
    if (r == SQLITE_CONSTRAINT_UNIQUE)
        fatal3("class ‘", new, "’ already exists");
    if (r != SQLITE_DONE) db_fatal("step", q);

    sqlite3_finalize(stmt);
}
