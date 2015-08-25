/*
 * skiplist.h:
 *
 * Copyright (c) 2003 Chris Lightfoot. All rights reserved.
 * Email: chris@ex-parrot.com; WWW: http://www.ex-parrot.com/~chris/
 *
 * $Id: skiplist.h,v 1.1 2003/01/16 10:45:28 chris Exp $
 *
 */

#ifndef __SKIPLIST_H_ /* include guard */
#define __SKIPLIST_H_

#include <sys/types.h>

typedef struct skiplist_node* skiplist_iterator;
typedef struct tag_skiplist* skiplist;

/* skiplist.c */
skiplist skiplist_new(int (*compare)(const void *, const size_t, const void *, const size_t));
void skiplist_delete(skiplist S);
void skiplist_delete_free(skiplist S, void (*freefunc)(void *));
int skiplist_insert_copy(skiplist S, const void *key, const size_t klen, const void *val, const size_t vlen);
int skiplist_insert(skiplist S, const void *key, const size_t klen, void *v);
size_t skiplist_size(skiplist S);
int skiplist_contains(skiplist S, const void *key, const size_t klen);
void *skiplist_find(skiplist S, const void *key, const size_t klen);
int skiplist_remove_copy(skiplist S, const void *key, const size_t klen);
void *skiplist_remove(skiplist S, const void *key, const size_t klen);
void *skiplist_itr_value(skiplist S, skiplist_iterator itr);
void *skiplist_itr_key(skiplist S, skiplist_iterator itr, size_t *len);
skiplist_iterator skiplist_itr_first(skiplist S);
skiplist_iterator skiplist_itr_last(skiplist S);
skiplist_iterator skiplist_itr_next(skiplist S, skiplist_iterator itr);
skiplist_iterator skiplist_itr_prev(skiplist S, skiplist_iterator itr);

#endif /* __SKIPLIST_H_ */
