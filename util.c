/*
 * util.c:
 * Useful functions.
 *
 * Copyright (c) 2003 Chris Lightfoot. All rights reserved.
 * Email: chris@ex-parrot.com; WWW: http://www.ex-parrot.com/~chris/
 *
 */

static const char rcsid[] = "$Id: util.c,v 1.1 2003/01/16 10:45:28 chris Exp $";

/* for strdup() */
#define _DEFAULT_SOURCE 1

#include <sys/types.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "util.h"

/* xmalloc:
 * Malloc, and abort if malloc fails. */
void *xmalloc(size_t n) {
    void *v;
    v = malloc(n);
    if (!v) abort();
    return v;
}

/* xcalloc:
 * As above. */
void *xcalloc(size_t n, size_t m) {
    void *v;
    v = calloc(n, m);
    if (!v) abort();
    return v;
}

/* xrealloc:
 * As above. */
void *xrealloc(void *w, size_t n) {
    void *v;
    v = realloc(w, n);
    if (n != 0 && !v) abort();
    return v;
}

/* xstrdup:
 * As above. */
char *xstrdup(const char *s) {
    char *t;
    t = strdup(s);
    if (!t) abort();
    return t;
}

/* xfree:
 * Free, ignoring a passed NULL value. */
void xfree(void *v) {
    if (v) free(v);
}

/* xread:
 * Read data, ignoring signals. */
ssize_t xread(int fd, void *buf, size_t count) {
    int i;
    do
        i = read(fd, buf, count);
    while (i == -1 && errno == EINTR);
    return i;
}

/* memstr HAYSTACK HLEN NEEDLE NLEN
 * Locate NEEDLE, of length NLEN, in HAYSTACK, of length HLEN, returning NULL
 * if it is not found. Uses the Boyer-Moore search algorithm. Cf.
 *  http://www-igm.univ-mlv.fr/~lecroq/string/node14.html */
const char *memstr(const char *haystack, const size_t hlen,
	     const char *needle, const size_t nlen) {
    int skip[256], k;

    if (nlen == 0) return (char*)haystack;

    /* Set up the finite state machine we use. */
    for (k = 0; k < 256; ++k) skip[k] = nlen;
    for (k = 0; k < nlen - 1; ++k)
        skip[(unsigned char)needle[k]] = nlen - k - 1;

    /* Do the search. */
    for (k = nlen - 1; k < hlen; k += skip[(unsigned char)haystack[k]]) {
        int i, j;
        for (j = nlen - 1, i = k; j >= 0 && haystack[i] == needle[j]; j--) i--;
        if (j == -1) return haystack + i + 1;
    }

    return NULL;
}

