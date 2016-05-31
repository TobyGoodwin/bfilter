/*
 * pool.c:
 * Pools of memory, allocated like GNU C's obstacks.
 *
 * Copyright (c) 2002 Chris Lightfoot. All rights reserved.
 * Email: chris@ex-parrot.com; WWW: http://www.ex-parrot.com/~chris/
 *
 */

#include <stdlib.h>
#include <string.h>

#include "pool.h"
#include "util.h"

static inline size_t max(const int a, const int b) {
    return a > b ? a : b;
}

static inline size_t min(const int a, const int b) {
    return a < b ? a : b;
}

/* pool_new
 * Create a new pool of default size. */
pool pool_new(void) {
    return pool_new_size(DEFSIZE);
}

/* pool_new_size SIZE
 * Create a new pool of specified size. */
pool pool_new_size(const size_t size) {
    pool p;
    alloc_struct(tag_pool, p);
    p->data = p->lastblock = xmalloc(p->len = size);
    p->cur = p;
    return p;
}

/* pool_delete POOL
 * Delete a pool, freeing all the contained data. */
void pool_delete(pool p) {
    pool q;
    do {
        q = p->next;
        xfree(p->data);
        xfree(p);
        p = q;
    } while (p);
}

/* pool_free POOL PTR
 * Because of the strategy we use, only the last block in a pool can be freed.
 * In any other case, we do nothing silently. */
void pool_free(pool p, void *v) {
    pool q;
    q = p->cur;
    if (v == q->lastblock && q->lastblock > q->data)
        q->lastlen = 0;
}

/* pool_malloc POOL SIZE
 * Like malloc(3). */
void *pool_malloc(pool p, size_t n) {
    pool q;
    void *v;
    
    /* Round up size to alignment. */
    n += ALIGN - (n % ALIGN);

    q = p->cur;
    
    /* Is there space in this block? If not, allocate a new block. */
    if (q->lastblock + q->lastlen + n > q->data + q->len) {
        q->next = pool_new_size(2 * max(n, q->len));
        p->cur = q = q->next;
    }
    
    q->lastblock += q->lastlen;
    v = q->lastblock;
    q->lastlen = n;

    return v;
}

/* pool_calloc POOL NELEM ELEMSIZE
 * Like calloc(3). */
void *pool_calloc(pool p, size_t n, size_t size) {
    void *v;
    v = pool_malloc(p, n * size);
    memset(v, 0, n * size);
    return v;
}

/* pool_realloc POOL PTR NEWSIZE
 * Like realloc(3). This is an inefficient operation. */
void *pool_realloc(pool p, void *v, size_t n) {
    pool q;

    q = p->cur;

    /* Round up size to alignment. */
    n += ALIGN - (n % ALIGN);

    if (v == q->lastblock && q->lastblock + n < q->data + q->len) {
        q->lastlen = n;
        return v;
    } else {
        void *v2;
        pool r;
        
        v2 = pool_malloc(p, n);
        
        /* Now need to copy the data from the old buffer to this buffer. This
         * means that we need to find out where the old buffer was, so that we
         * can figure out how much data to copy. */
        if ((char*)v >= q->data && (char*)v < q->lastblock + q->lastlen)
            /* Optimise if it is in this buffer. */
            r = q;
        else
            /* Locate buffer. */
            for (r = p; r != q; r = r->next)
                if ((char*)v >= r->data && (char*)v < r->lastblock + r->lastlen)
                    break;

        memcpy(v2, v, min(n, r->lastblock + r->lastlen - (char*)v));

        return v2;
    }
}

/* pool_strdup POOL STRING
 * Like strdup(3). */
char *pool_strdup(pool p, const char *s) {
    char *t;
    t = pool_malloc(p, strlen(s) + 1);
    return strcpy(t, s);
}
