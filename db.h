/*
 * db.h:
 * bfilter database interface.
 *
 * Copyright (c) 2004 Chris Lightfoot. All rights reserved.
 * Email: chris@ex-parrot.com; WWW: http://www.ex-parrot.com/~chris/
 *
 * $Id: db.h,v 1.3 2004/06/07 15:12:02 chris Exp $
 *
 */

#ifndef __DB_H_ /* include guard */
#define __DB_H_

/* db.c */
int db_open(void);
void db_close(void);
void db_set_pair(const char *name, int a, int b);
int db_get_pair(const char *name, int *a, int *b);
unsigned int db_clean(int ndays);
void db_print_stats(void);

#endif /* __DB_H_ */
