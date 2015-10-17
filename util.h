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

#ifndef __UTIL_H_
#define __UTIL_H_

#include <stdint.h>
#include <unistd.h>

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
const uint8_t *memstr(const uint8_t *haystack, const size_t hlen,
	const uint8_t *needle, const size_t nlen);

#endif /* __UTIL_H_ */
