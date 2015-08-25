/*
 * util.h:
 * Useful macros and function prototypes.
 *
 * Copyright (c) 2003 Chris Lightfoot. All rights reserved.
 * Email: chris@ex-parrot.com; WWW: http://www.ex-parrot.com/~chris/
 *
 * $Id: util.h,v 1.1 2003/01/16 10:45:28 chris Exp $
 *
 */

#ifndef __UTIL_H_ /* include guard */
#define __UTIL_H_

/* alloc_struct TAG P
 * Enough memory to hold a struct TAG is allocated and assigned to P, and the
 * memory is initialised so that all numeric elements of *P are zero and all
 * pointer elements NULL. */
#define alloc_struct(tag, p)                    \
            do {                                \
                struct tag aszero ## tag = {0}; \
                p = malloc(sizeof *p);          \
                *p = aszero ## tag;             \
            } while (0)

/* realloc_if_needed PTR N
 * Reallocates PTR, which presently holds N * sizeof *PTR elements to hold at
 * least N + 1 elements. Allocates storage in power-of-two blocks. Evaluates
 * PTR and N more than once. */
#define realloc_if_needed(ptr, n)                           \
            if (((n) & ((n) - 1)) == 0)                     \
                (ptr) = xrealloc((ptr), ((n) ? (n) * 2 : 1) * sizeof *(ptr))

/* util.c */
void *xmalloc(size_t n);
void *xcalloc(size_t n, size_t m);
void *xrealloc(void *w, size_t n);
char *xstrdup(const char *s);
void xfree(void *v);
ssize_t xread(int fd, void *buf, size_t count);
unsigned char *memstr(const unsigned char *haystack, const size_t hlen, const unsigned char *needle, const size_t nlen);

#endif /* __UTIL_H_ */
