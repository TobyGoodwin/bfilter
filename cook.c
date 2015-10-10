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

#include <stdint.h>
#include <string.h>

#include "cook.h"
#include "line.h"

/* unbase64 CHAR
 * Decode a single base64 CHAR. */
static uint_least32_t unbase64(char c) {
    static const char *s = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    if (c == '=')
        return 0;
    else
        return (uint_least32_t)(strchr(s, c) - s);
}

/* decode_base64 BUFFER LENGTH
 * Decode LENGTH characters of base64 data starting at BUFFER in place,
 * returning the number of bytes of real data produced, and padding the
 * end of the original extent with whitespace. */
/* XXX what is the point of the whitespace padding? */
static size_t decode_base64(char *buf, size_t len) {
    char *rd, *wr;
    int nuls = 0;

    nuls = (buf[len - 1] == '=') + (buf[len - 2] == '=');
    for (rd = buf, wr = buf; rd + 3 < buf + len; rd += 4, wr += 3) {
        uint_least32_t X;
        X = unbase64(rd[3]) | (unbase64(rd[2]) << 6) | (unbase64(rd[1]) << 12) | (unbase64(rd[0]) << 18);
        wr[2] = X & 0xff;
        wr[1] = (X >> 8) & 0xff;
        wr[0] = (X >> 16) & 0xff;
    }

    memset(wr, ' ', len - (wr - buf));

    return wr - buf - nuls;
}

void cook_b64(struct line *t) {
    /* if we do not have a multiple of 4 bytes, this probably wasn't
     * base64 after all */
    if (t->l % 4 != 0)
        return;
    t->l = decode_base64(t->x, t->l);
}

void cook_header(struct line *t) {
    char *p;

    /* move past field name */
    if ((p = memchr(t->x, ':', t->l))) {
        ++p;
        memmove(t->x, p, t->x + t->l - p);
        t->l -= p - t->x;
    }
}

#define is_qp_digit(x) ((x>='0' && x<='9') || (x>='A' && x<='F'))
#define qp_digit_val(x) (x>'9' ? x+10-'A' : x-'0')
#define qp_digits_val(p) (16*qp_digit_val(*(p)) + qp_digit_val(*((p)+1)))

static size_t decode_qp(char *buf, size_t len) {
    char *rd, *wr;

    for (rd = buf, wr = buf; rd < buf + len; ++wr) {
        if (rd[0] == '=' && rd + 2 < buf + len &&
            is_qp_digit(rd[1]) && is_qp_digit(rd[2])) {
            *wr = qp_digits_val(rd + 1);
            rd += 3;
        } else
            *wr = *rd++;
    }
    return wr - buf;
}

void cook_qp(struct line *t) {
    t->l = decode_qp(t->x, t->l);
}
