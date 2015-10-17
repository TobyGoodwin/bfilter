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

#include <assert.h>
#include <stdint.h>

#include "utf8.h"

/* write the utf-8 encoding of c to s, and return its length. s is assumed to
 * have enough space */
int utf8_encode(uint8_t *s, unsigned int c) {
    if (c < 0x80) {
        s[0] = c;
        return 1;
    }
    if (c < 0x800) {
        s[0] = 0xc0 + (c >> 6);
        s[1] = 0x80 + (c & 0x3f);
        return 2;
    }
    if (c < 0x10000) {
        s[0] = 0xe0 + (c >> 12);
        s[1] = 0x80 + (c >> 6 & 0x3f);
        s[2] = 0x80 + (c & 0x3f);
        return 3;
    }
    if (c < 0x110000) {
        s[0] = 0xf0 + (c >> 18);
        s[1] = 0x80 + (c >> 12 & 0x3f);
        s[2] = 0x80 + (c >> 6 & 0x3f);
        s[3] = 0x80 + (c & 0x3f);
        return 4;
    }

    assert(0); /* caller validates */
    return 0;
}
