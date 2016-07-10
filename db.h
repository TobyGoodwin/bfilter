/*

    Copyright (c) 2004 Chris Lightfoot. All rights reserved.
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

#ifndef __DB_H_
#define __DB_H_

#include <sqlite3.h>

sqlite3 *db_db(void);
_Bool db_read(void);
void db_write(void);
void db_fatal(const char *, const char *);
void db_close(void);

void db_begin(void);
void db_commit(void);
void db_vacuum(void);

int db_int_query(const char *, size_t);
int db_documents(void);
int db_vocabulary(void);

struct db_stmt {
    const char *s;
    size_t n;
    sqlite3_stmt *x;
};
int db_stmt_ready(struct db_stmt *);
void db_stmt_finalize(struct db_stmt *);

void db_class_rename(const char *, const char *);

void db_purge(void);

#endif /* __DB_H_ */
