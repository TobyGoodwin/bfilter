#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

#include "line.h"
#include "util.h"

void line_read(FILE *in, struct line *l) {
    int i;

    l->l = 0;
    while ((i = getc(in)) != EOF) {
        if (l->l == l->a)
            l->x = xrealloc(l->x, l->a += l->a + 1);
        l->x[l->l++] = (char)i;
        if (i == '\n') break;
    }
}

_Bool line_write(FILE *out, struct line *l) {
    return fwrite(l->x, 1, l->l, out) == l->l;
}

void line_cat(struct line *dst, struct line *src) {
    size_t len = dst->l + src->l;

    if (len > dst->a)
        dst->x = xrealloc(dst->x, dst->a = len);
    memcpy(dst->x + dst->l, src->x, src->l);
    dst->l = len;
}

void line_copy(struct line *dst, struct line *src) {
    dst->l = 0;
    line_cat(dst, src);
}

_Bool line_blank(struct line *l) {
    return l->l == 1 && l->x[0] == '\n';
}

_Bool line_empty(struct line *l) {
    return l->l == 0;
}

/* Is this line composed only of base64 characters? */
_Bool line_is_b64(struct line *l) {
    const char *p;
    size_t len = l->l;

    /* must be at least 4 encoded characters + \n */
    if (len < 5)
        return 0;

    /* backup over terminating \n */
    assert(len > 0 && l->x[len - 1] == '\n');
    --len;

    /* back up past terminating =s */
    while (len > 0 && l->x[len - 1] == '=')
        --len;

    for (p = l->x; p < l->x + len; ++p)
        if (!((*p >= 'A' && *p <= 'Z')
               || (*p >= 'a' && *p <= 'z')
               || (*p >= '0' && *p <= '9')
               || *p == '+' || *p == '/'))
            return 0;

    return 1;
}

_Bool line_starts(struct line *l, const char *m) {
    size_t len = strlen(m);

    return l->l >= len && strncmp(l->x, m, len) == 0;
}

_Bool line_starts_ci(struct line *l, const char *m) {
    size_t len = strlen(m);

    return l->l >= len && strncasecmp(l->x, m, len) == 0;
}

_Bool line_ends(struct line *l, const char *m) {
    size_t len = strlen(m);

    return l->l >= len && strncasecmp(l->x + l->l - len, m, len) == 0;
}

_Bool line_hdr_cont(struct line *l) {
    return line_starts(l, " ") || line_starts(l, "\t");
}
