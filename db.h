/*

    Copyright (c) 2004 Chris Lightfoot. All rights reserved.
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

#ifndef __DB_H_
#define __DB_H_

#include <sqlite3.h>

#include "class.h"

/* db.c */
sqlite3 *db_db(void);
int db_open(void);
void db_close(void);

void db_begin(void);
void db_commit(void);

uint8_t *db_hash_fetch(uint8_t *k, size_t k_sz, size_t *d_sz);
_Bool db_hash_store(uint8_t *k, size_t k_sz, uint8_t *d, size_t d_sz);

uint32_t *db_hash_fetch_uint32(uint8_t *k, size_t k_sz);
_Bool db_hash_store_uint32(uint8_t *k, size_t k_sz, uint32_t d);

#if 0
void db_set_pair(const char *name, unsint a, int b);
int db_get_pair(const char *name, int *a, int *b);
#endif

void db_set_intlist(const uint8_t *, size_t, uint32_t *, unsigned int);
uint32_t *db_get_intlist(const uint8_t *, size_t, unsigned int *);
void db_clean(int ndays);
void db_print_stats(void);

#endif /* __DB_H_ */
