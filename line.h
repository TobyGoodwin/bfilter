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

#ifndef LINE_H
#define LINE_H 1

#include <stdint.h>
#include <stdio.h>

struct line {
    uint8_t *x;  /* contents */
    size_t l; /* length */
    size_t a; /* allocated */
};

void line_read(FILE *, struct line *);
_Bool line_write(FILE *out, struct line *l);

void line_cat(struct line *, struct line *);
void line_copy(struct line *, struct line *);

_Bool line_blank(struct line *);
_Bool line_empty(struct line *);
_Bool line_ends(struct line *, const char *);
_Bool line_hdr_cont(struct line *);
_Bool line_is_b64(struct line *);
_Bool line_starts(struct line *, const char *);
_Bool line_starts_ci(struct line *, const char *);

#endif
