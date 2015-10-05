#include <stdio.h>

#include "bfilter.h"
#include "read.h"
#include "util.h"

struct line {
    char *x;  /* contents */
    size_t l; /* length */
    size_t a; /* allocated */
};

enum state { hdr, bdy, end };

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

enum state transition(enum state s) {
    return s;
}

_Bool read_email(const _Bool fromline, FILE *in, FILE **tmp) {
    enum state s_old, s_new;
    struct line l = { 0 };

    s_old = hdr;

    /* If tmp is set, we are in "passthrough" mode; *tmp is the FILE * where we
     * will stash the body of the email, to be output later. */
    if (tmp && !(*tmp = tmpfile()))
            return 0;

    while (1) {
        read_line(in, &l);
        //s_new = transition(s_old);
        if (l.l == 0) break;
        if (tmp && !write_line(stdout, &l))
            goto abort;
    }
    return 1;

abort:
    if (*tmp)
        fclose(*tmp);
    return 0;
}
