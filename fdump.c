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

#include <errno.h>
#include <stdio.h>

#include "fdump.h"

/* Dumps an open file to standard output. */
_Bool fdump(FILE *f) {
    rewind(f);
    do {
        unsigned char buf[8192];
        size_t n;

        n = fread(buf, 1, 8192, f);
        if (ferror(f) || (n > 0 && fwrite(buf, 1, n, stdout) != n))
            break;
    } while (!feof(f) && !ferror(f));

    if (ferror(f)) {
        fprintf(stderr, "bfilter: temporary file: read error (%s)\n",
                strerror(errno));
        return 0;
    }

    fflush(stdout);
    if (ferror(stdout)) {
        fprintf(stderr, "bfilter: standard output: write error (%s)\n",
                strerror(errno));
        return 0;
    }

    return 1;
}
