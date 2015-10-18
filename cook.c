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
#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "cook.h"
#include "line.h"
#include "utf8.h"
#include "util.h"

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
static size_t decode_base64(uint8_t *buf, size_t len) {
    uint8_t *rd, *wr;
    int nuls = 0;

    nuls = (buf[len - 1] == '=') + (buf[len - 2] == '=');
    for (rd = buf, wr = buf; rd + 3 < buf + len; rd += 4, wr += 3) {
        uint_least32_t X;
        X = unbase64(rd[3]) | (unbase64(rd[2]) << 6) |
            (unbase64(rd[1]) << 12) | (unbase64(rd[0]) << 18);
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

#define hex_digit_val(x) (x>'9' ? x + 10 - (x>'F' ? 'a' : 'A') : x - '0')
static _Bool decode_entity(uint8_t *s, size_t len, int *result, int *size) {
    _Bool hex = *s == 'x';
    uint8_t *p;
    int a = 0;

    for (p = s + hex; (hex ? isxdigit(*p) : isdigit(*p))
            && p < s + len && a < 0x110000; ++p)
        a = a * (hex ? 16 : 10) + (hex ? hex_digit_val(*p) : *p - '0');
    if (p == s + len || *p != ';' || a > 0x10ffff)
        return 0;

    *result = a;
    *size = p - s;
    return 1;
}

/* decode numeric html entities into utf-8. the utf-8 representation must be
 * shorter than the entity, since entities encode at most 4 bits per byte with
 * an overhead of 3 or 4 bytes, whereas utf-8 encodes 6 bits per byte with
 * an overhead of 1 byte */
static size_t decode_entities(uint8_t *buf, size_t len) {
    uint8_t *rd, *wr;

    for (rd = buf, wr = buf; rd < buf + len; ++rd, ++wr) {
        if (rd + 4 < buf + len && rd[0] == '&' && rd[1] == '#') {
            int c, sz;
            if (decode_entity(rd + 2, len - (rd - buf), &c, &sz)) {
                wr += utf8_encode(wr, c) - 1;
                rd += sz + 2; /* 2 for "&#" */
            }
        } else
            *wr = *rd;
    }

    return wr - buf;
}

void cook_entities(struct line *t) {
    t->l = decode_entities(t->x, t->l);
}

/* heuristic check for text: is the first 1k of the string free from NULs? */
_Bool is_text(struct line *t) {
    uint8_t *p;
    size_t l = 1024;
    
    if (l > t->l) l = t->l;
    for (p = t->x; p < t->x + l; ++p)
        if (*p == '\0')
            return 0;
    return 1;
}

#define is_qp_digit(x) ((x>='0' && x<='9') || (x>='A' && x<='F'))
#define qp_digit_val(x) (x>'9' ? x+10-'A' : x-'0')
#define qp_digits_val(p) (16*qp_digit_val(*(p)) + qp_digit_val(*((p)+1)))

static size_t decode_qp(uint8_t *buf, size_t len) {
    uint8_t *rd, *wr;

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

void cook_header(struct line *t) {
    uint8_t *p, *q, *r, *s, *u, *v;
    enum { other, utf8, iso8859_1 } cs = other;
    enum { b64, qp } enc;
    size_t l;

    /* move past field name */
    if ((p = memchr(t->x, ':', t->l))) {
        ++p;
        memmove(t->x, p, t->x + t->l - p);
        t->l -= p - t->x;
    }

    for (p = t->x;
            (p = (uint8_t *)memstr(p, t->l - (p-t->x), (uint8_t *)"=?", 2)) &&
            (q = (uint8_t *)memstr(p+2, t->l - (p-t->x), (uint8_t *)"?", 1)) &&
            (r = (uint8_t *)memstr(q+1, t->l - (p-t->x), (uint8_t *)"?", 1)) &&
            (s = (uint8_t *)memstr(r+1, t->l - (p-t->x), (uint8_t *)"?=", 2));
            p = u + l) {
        u = p;
        /* if we don't decode anything here, move on past =? */
        l = 2;
        p += 2;
        if (p + 6 < t->x + t->l &&
                strncasecmp((char *)p, "utf-8?", 6) == 0) {
            p += 6;
            cs = utf8;
        } else if (p + 11 < t->x + t->l &&
                strncasecmp((char *)p, "iso-8859-1?", 11) == 0) {
            p += 11;
            cs= iso8859_1;
        }
        if (cs == other) continue;
        if (*p == 'q' || *p == 'Q') enc = qp;
        else if (*p == 'b' || *p == 'B') enc = b64;
        else continue;

        if (*++p != '?') continue;
        ++p;
        v = s + 2;
        assert(p <= s);

        /* we now have a piece of encoded data in [p, s) with the whole
         * encoded-word running as [u, v). start by decoding in place */
        switch (enc) {
            uint8_t *x;

            case qp:
                for (x = p; x < s; ++x)
                    if (*x == '_') *x = ' ';
                l = decode_qp(p, s - p);
                break;
            case b64:
                l = decode_base64(p, s - p);
                break;
        }

        switch (cs) {
            case utf8: /* simples! */
                /* move decoded text down */
                memmove(u, p, l);
                /* move remainder down */
                memmove(u + l, v, t->x + t->l - v);
                /* adjust length */
                t->l -= (v - u) - l;
                break;

            case iso8859_1: /* yeuch! */
                {
                    struct line n;
                    uint8_t *x;

                    n.a = 2 * t->l;
                    n.x = xmalloc(n.a);
                    /* initial segment */
                    n.l = u - t->x;
                    u = n.x + n.l;
                    memcpy(n.x, t->x, n.l);
                    /* encode iso-8859-1 to utf-8 */
                    for (x = p; x < p + l; ++x)
                        n.l += utf8_encode(n.x + n.l, *x);
                    /* trailing segment */
                    memcpy(n.x + n.l, v, t->x + t->l - v);
                    n.l += t->x + t->l - v;
                    /* switch */
                    free(t->x);
                    t->x = n.x;
                    t->l = n.l;
                    t->a = n.a;
                    break;
                }

            default:
                assert(0);
                break;
        }
    }
}

void cook_qp(struct line *t) {
    t->l = decode_qp(t->x, t->l);
}
