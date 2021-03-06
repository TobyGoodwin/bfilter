/*

    Copyright (c) 2003 Chris Lightfoot. All rights reserved.
    Copyright (c) 2015 - 2016 Toby Goodwin.
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

#include <stdint.h>

struct class *class_fetch(int *, int **);
int class_id_furnish(char *);
void class_update(int, int, int);

struct class {
    int id; /* unique id */
    const uint8_t *name; /* nul terminated */
    int docs; /* number of documents in this class */
    int terms; /* total number of terms (inc dups) in docs in class */
    double logprob; /* log(p) for this class when testing */
};
