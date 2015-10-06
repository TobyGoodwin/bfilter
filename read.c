#include <stdio.h>
#include <string.h>
#include <strings.h>

#include "bfilter.h"
#include "read.h"
#include "util.h"

struct line {
    char *x;  /* contents */
    size_t l; /* length */
    size_t a; /* allocated */
};

enum state { hdr, hdr_rel, bdy, end };

void read_line(FILE *in, struct line *l) {
    int i;

    l->l = 0;
    while ((i = getc(in)) != EOF) {
        if (l->l == l->a)
            l->x = xrealloc(l->x, l->a += l->a + 1);
        l->x[l->l++] = (char)i;
        ++nbytesrd;
        if (i == '\n') break;
    }
}

_Bool write_line(FILE *out, struct line *l) {
    return fwrite(l->x, 1, l->l, out) == l->l;
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

enum state transition(enum state s, struct line *l) {
    if (line_empty(l))
        return end;
    switch (s) {
        case bdy:
            break;

        case hdr:
            if (line_blank(l))
                return bdy;
            if (line_starts(l, "subject:"))
                return hdr_rel;
            break;

        case hdr_rel:
            if (line_blank(l))
                return bdy;
            if (line_hdr_cont(l))
                return hdr_rel;
            return hdr;

        case end:
            break;
    }
    return s;
}

_Bool read_email(const _Bool fromline, FILE *in, FILE **tmp) {
    enum state s_old, s_new;
    /* One line to Tokenize, and one to Xmit */
    struct line t = { 0 }, x = { 0 };

    s_old = hdr;

    /* If tmp is set, we are in "passthrough" mode; *tmp is the FILE * where we
     * will stash the body of the email, to be output later. */
    if (tmp && !(*tmp = tmpfile()))
            return 0;

    while (1) {
        read_line(in, &x);
        s_new = transition(s_old, &x);
        if (s_new == end) break;
        if (tmp && !write_line(stdout, &x))
            goto abort;
    }
    return 1;

abort:
    if (*tmp)
        fclose(*tmp);
    return 0;
}
