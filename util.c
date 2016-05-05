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

/* for strdup() */
#define _DEFAULT_SOURCE 1

#include <sys/types.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "error.h"
#include "util.h"

/* xmalloc:
 * Malloc, and abort if malloc fails. */
void *xmalloc(size_t n) {
    void *v;
    v = malloc(n);
    if (!v) nomem();
    return v;
}

/* xcalloc:
 * As above. */
void *xcalloc(size_t n, size_t m) {
    void *v;
    v = calloc(n, m);
    if (!v) nomem();
    return v;
}

/* xrealloc:
 * As above. */
void *xrealloc(void *w, size_t n) {
    void *v;
    v = realloc(w, n);
    if (n != 0 && !v) nomem();
    return v;
}

/* xstrdup:
 * As above. */
char *xstrdup(const char *s) {
    char *t;
    t = strdup((char *)s);
    if (!t) nomem();
    return t;
}

uint8_t *u8_xstrdup(const uint8_t *s) {
    return (uint8_t *)xstrdup((const char *)s);
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
const uint8_t *memstr(const uint8_t *haystack, const size_t hlen,
	     const uint8_t *needle, const size_t nlen) {
    int skip[256], k;

    if (nlen == 0) return haystack;

    /* Set up the finite state machine we use. */
    for (k = 0; k < 256; ++k) skip[k] = nlen;
    for (k = 0; k < nlen - 1; ++k)
        skip[needle[k]] = nlen - k - 1;

    /* Do the search. */
    for (k = nlen - 1; k < hlen; k += skip[haystack[k]]) {
        int i, j;
        for (j = nlen - 1, i = k; j >= 0 && haystack[i] == needle[j]; j--)
            i--;
        if (j == -1) return haystack + i + 1;
    }

    return NULL;
}

_Bool prefix(const char *x, const char *y) {
    return strncmp(x, y, strlen(x)) == 0;
}
