/*
 * pool.h:
 * Pools of memory, allocated like GNU C's obstacks.
 *
 * Copyright (c) 2002 Chris Lightfoot. All rights reserved.
 * Email: chris@ex-parrot.com; WWW: http://www.ex-parrot.com/~chris/
 *
 * $Id: pool.h,v 1.1 2003/01/16 10:45:28 chris Exp $
 *
 */

#ifndef __POOL_H_ /* include guard */
#define __POOL_H_

#include <sys/types.h>

#define ALIGN       4
#define DEFSIZE     256

/* pool:
 * Memory pool. */
typedef struct tag_pool {
    size_t len, lastlen;
    char *data, *lastblock;
    struct tag_pool *next, *cur;
} *pool;

/* pool.c */
pool pool_new(void);
pool pool_new_size(const size_t size);
void pool_delete(pool p);

void pool_free(pool p, void *v);
void *pool_malloc(pool p, size_t n);
void *pool_calloc(pool p, size_t n, size_t size);
void *pool_realloc(pool p, void *v, size_t n);
char *pool_strdup(pool p, const char *s);

#endif /* __POOL_H_ */
