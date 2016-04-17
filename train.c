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
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "bfilter.h"
#include "count.h"
#include "db.h"
#include "error.h"
#include "read.h"
#include "skiplist.h"
#include "submit.h"
#include "train.h"
#include "util.h"

#define TRACE if (0)

_Bool train_read(void) {
    do {
        errno = 0;
        ++nemails;
        if (!read_email(flagb, stdin, 0)) {
            fprintf(stderr, "bfilter: error while reading email (%s)\n",
                    errno ? strerror(errno) : "no system error");
            return 0;
        }

        /* If we're running on a terminal, print stats. */
        if (isatty(1))
            fprintf(stderr,
        "Reading: %8u emails (%8u bytes) %8lu terms avg length %8.2f\r",
                nemails, (unsigned)nbytesrd, skiplist_size(token_list),
                (double)term_length / skiplist_size(token_list));
    } while (!feof(stdin));
    if (isatty(1))
        fprintf(stderr, "\n");
    return 1;
}

const char begin[] = "BEGIN TRANSACTION;";
const char commit[] = "COMMIT;";
const char insert_class[] = "\
INSERT INTO class (name, docs, terms) \
  VALUES (?, 0, 0);\
";
#if 0
const char *insert_class = "\
INSERT INTO class (name, docs, terms) \
  VALUES (?, 0, 0); \
";
#endif
const char *get_class = "SELECT id FROM class WHERE name = ?;";
const char *update_class = "\
UPDATE class \
  SET docs = docs + ?, terms = terms + ? \
  WHERE name = ?; \
";

void train_update(char *cclass) {
    /* Update total number of emails and the data for each word. */
    char *t;
    sqlite3 *db = db_db();
    struct class *classes, *tclass;
    int cid;
    uint32_t Ndb, *pNdb;
    uint32_t nvocab, *pnvocab;
    skiplist_iterator si;
    unsigned int nterms, ntermswr, ntermsnew, ntermsall;
    _Bool inserted = 0;

    int r, v;
    sqlite3_stmt *stmt;
    char *errmsg;

#if 0
    r = sqlite3_exec(db, begin, 0, 0, &errmsg);
    if (errmsg) fatal2("cannot begin transaction: ", errmsg);

    r = sqlite3_prepare_v2(db, get_class, strlen(get_class), &stmt, 0);
    if (r != SQLITE_OK)
        fatal2("cannot prepare statement: ", sqlite3_errmsg(db));

loop:
    r = sqlite3_bind_text(stmt, 1, cclass, strlen(cclass), 0);
    if (r != SQLITE_OK)
        fatal2("cannot bind value: ", sqlite3_errmsg(db));
    r = sqlite3_step(stmt);
    if (r == SQLITE_ROW) {
        if (sqlite3_column_type(stmt, 0) != SQLITE_INTEGER)
            fatal1("class.name has non-integer type");
        cid = sqlite3_column_int(stmt, 0);
    } else if (r == SQLITE_DONE) {
        sqlite3_stmt *s;
        assert(!inserted); inserted = 1;
        // insert
        r = sqlite3_prepare_v2(db, insert_class, sizeof(insert_class), &s, 0);
        if (r != SQLITE_OK)
            fatal4("cannot prepare statement `", insert_class, "': ",
                    sqlite3_errmsg(db));
        r = sqlite3_bind_text(s, 1, cclass, strlen(cclass), 0);
        if (r != SQLITE_OK)
            fatal2("cannot bind value: ", sqlite3_errmsg(db));
        r = sqlite3_step(s);
        if (r != SQLITE_DONE)
            fatal2("cannot step statement: ", sqlite3_errmsg(db));
        sqlite3_finalize(s);
        sqlite3_reset(stmt);
        goto loop;
    } else
        fatal2("cannot step statement: ", sqlite3_errmsg(db));
    sqlite3_finalize(stmt);

    r = sqlite3_exec(db, commit, 0, 0, &errmsg);
    if (errmsg) fatal2("cannot commit: ", errmsg);
#endif

    cid = class_id_furnish(cclass);

fprintf(stderr, "cid is %d\n", cid);
    //
    // classes = class_fetch();
    // tclass = class_lookup(classes, cclass);
   
    // select from class where name = cclass;
    // insert if missing

    // not needed with sqlite
    // pNdb = db_hash_fetch_uint32((uint8_t *)KEY_DOCUMENTS,
    //         sizeof KEY_DOCUMENTS - 1);
    // if (pNdb) Ndb = *pNdb;
    // else Ndb = 0;
    // Ndb += nemails;
    // db_hash_store_uint32((uint8_t *)KEY_DOCUMENTS, sizeof KEY_DOCUMENTS - 1,
    //         Ndb);

    // tclass->docs += nemails;

    // if (isatty(1))
    //     fprintf(stderr, "Writing: corpus now contains %u emails\n", Ndb);

    nterms = skiplist_size(token_list); /* distinct terms */
    ntermsall = 0; /* terms including dups */

    for (si = skiplist_itr_first(token_list), ntermswr = 0, ntermsnew = 0; si;
            si = skiplist_itr_next(token_list, si), ++ntermswr) {
        uint8_t *k;
        int *p;
        size_t kl;

        k = skiplist_itr_key(token_list, si, &kl);
        p = skiplist_itr_value(token_list, si);
if (0) fprintf(stderr, "term %.*s: %d\n", (int)kl, k, *p);
        // if (count_add(k, kl, tclass->code, *p))
        //    ++ntermsnew;
        ntermsall += *p;

        if (isatty(1) && (ntermswr % 500) == 0)
            fprintf(stderr, "Writing: %u / %u terms (%u new)\r",
                    ntermswr, nterms, ntermsnew);
    }

    if (isatty(1))
        fprintf(stderr, "Writing: %u / %u terms (%u new)\n",
                ntermswr, nterms, ntermsnew);

    r = sqlite3_prepare_v2(db, update_class, strlen(update_class), &stmt, 0);
    if (r != SQLITE_OK)
        fatal2("cannot prepare statement: ", sqlite3_errmsg(db));

    r = sqlite3_bind_int(stmt, 1, nemails);
    if (r != SQLITE_OK) fatal2("cannot bind value: ", sqlite3_errmsg(db));
    r = sqlite3_bind_int(stmt, 2, ntermsall);
    if (r != SQLITE_OK) fatal2("cannot bind value: ", sqlite3_errmsg(db));
    r = sqlite3_bind_text(stmt, 3, cclass, strlen(cclass), 0);
    if (r != SQLITE_OK) fatal2("cannot bind value: ", sqlite3_errmsg(db));

    r = sqlite3_step(stmt);
    if (r != SQLITE_DONE)
        fatal2("cannot update: ", sqlite3_errmsg(db));
    
    // not needed with sqlite
    // pnvocab = db_hash_fetch_uint32((uint8_t *)KEY_VOCABULARY,
    //         sizeof KEY_VOCABULARY - 1);
    // if (pnvocab) nvocab = *pnvocab;
    // else nvocab = 0;
    // TRACE fprintf(stderr, "vocabulary was: %u\n", nvocab);
    // nvocab += ntermsnew;
    // db_hash_store_uint32((uint8_t *)KEY_VOCABULARY,
    //         sizeof KEY_VOCABULARY - 1, nvocab);

    // update class set docs = docs + nemails, terms = terms + ntermsall where name = cclass;
    // tclass->terms += ntermsall;
    // class_store(classes);
}
