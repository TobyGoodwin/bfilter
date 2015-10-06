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

_Bool line_blank(struct line *l) {
    return l->l == 1 && l->x[0] == '\n';
}

_Bool line_empty(struct line *l) {
    return l->l == 0;
}

_Bool line_starts(struct line *l, const char *m) {
    size_t len = strlen(m);

    return l->l >= len && strncasecmp(l->x, m, len) == 0;
}

_Bool line_hdr_cont(struct line *l) {
    return line_starts(l, " ") || line_starts(l, "\t");
}
