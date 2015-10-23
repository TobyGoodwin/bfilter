/*

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

#include <stdint.h>
#include <string.h>

#include "count.h"
#include "db.h"
#include "util.h"

/* add n to the count for token t of size t_sz and class (code) c, return true
 * if this was a new term */
_Bool count_add(uint8_t *t, size_t t_sz, int c, int n) {
    _Bool gotit = 0;
    int i, n_is;
    uint32_t *is, *x;

    is = db_get_intlist(t, t_sz, &n_is);
    if (!is || n_is % 2 != 0) {
        x = xmalloc(2 * sizeof *x);
        x[0] = c;
        x[1] = n;
        db_set_intlist(t, t_sz, x, 2);
        return 1;
    }
    for (i = 0; !gotit && i < n_is; i += 2) {
        if (is[i] == c) {
            gotit = 1;
            is[i + 1] += n;
        }
    }
    if (gotit)
        x = is;
    else {
        int new = n_is + 2;
        x = xmalloc(new * sizeof *x);
        memcpy(x, is, n_is * sizeof *x);
        x[n_is] = c;
        x[n_is + 1] = n;
        n_is = new;
    }
    db_set_intlist(t, t_sz, x, n_is);
    return 0;
}
