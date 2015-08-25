/*
 * db.h:
 * bfilter database interface.
 *
 * Copyright (c) 2004 Chris Lightfoot. All rights reserved.
 * Email: chris@ex-parrot.com; WWW: http://www.ex-parrot.com/~chris/
 *
 * $Id: db.h,v 1.1 2004/04/11 17:14:04 chris Exp $
 *
 */

#ifndef __DB_H_ /* include guard */
#define __DB_H_

/* db.c */
int db_open(void);
void db_close(void);
void db_set_pair(const unsigned char *name, unsigned int a, unsigned int b);
int db_get_pair(const unsigned char *name, unsigned int *a, unsigned int *b);
unsigned int db_clean(int ndays);

#endif /* __DB_H_ */
