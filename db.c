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

static sqlite3 *db;

struct sqlite3 *db_db(void) {
    return db;
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
    if (r != SQLITE_OK)
        fatal2("cannot prepare statement: ", sqlite3_errmsg(db));
    r = sqlite3_bind_int(stmt, 1, VERSION);
    if (r != SQLITE_OK)
        fatal2("cannot bind value: ", sqlite3_errmsg(db));
    r = sqlite3_step(stmt);
    if (r != SQLITE_DONE)
        fatal2("cannot step statement: ", sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
}

int db_int_query(const char *q, size_t qn) {
    int r, x;
    sqlite3_stmt *stmt;

    r = sqlite3_prepare_v2(db, q, qn, &stmt, 0);
    if (r != SQLITE_OK)
        fatal4("cannot prepare statement `", q, "': ", sqlite3_errmsg(db));
    r = sqlite3_step(stmt);
    if (r != SQLITE_ROW)
        fatal4("cannot step statement `", q, "': ", sqlite3_errmsg(db));
    if (sqlite3_column_type(stmt, 0) != SQLITE_INTEGER)
        fatal3("result of `", q, "' has non-integer type");
    x = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    return x;
}

static const char get_version[] = "SELECT version FROM version;";

void db_check_version(void) {
    int v;

    v = db_int_query(get_version, sizeof get_version);
    if (v < MIN_VERSION || v > VERSION) {
        char vs[25];
        snprintf(vs, 25, "%d", v);
        fatal2("bad database version: ", vs);
    }
}

/*
    v = db_hash_fetch_uint32((uint8_t *)KEY_VERSION, sizeof KEY_VERSION - 1);
    if (!v || *v < MIN_VERSION || *v > VERSION)
        fatal1("bad database version");
*/

/* db_open
 * Open the filter database, which lives in ~/.bfildb. Returns 1 on success
 * or 0 on failure. Because we need to save timestamps, the database is opened
 * read/write in all cases. */
int db_open(void) {
    char *name;
    int err, flags;

    name = dbfilename(NULL);

    // db = tdb_open(name, 0, 0, O_RDWR, 0600);
    flags = SQLITE_OPEN_READWRITE;
    err = sqlite3_open_v2(name, &db, flags, 0);

    if (err != SQLITE_OK && errno == ENOENT) {
        flags |= SQLITE_OPEN_CREATE;
        err = sqlite3_open_v2(name, &db, flags, 0);
        if (err == SQLITE_OK)
            db_init();
    }

    if (err != SQLITE_OK) {
        fatal4("cannot open database `", name, "': ", sqlite3_errmsg(db));
        sqlite3_close(db);
    }

    db_check_version();

    return 1;
}

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
