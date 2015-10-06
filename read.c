#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

#include "bfilter.h"
#include "line.h"
#include "read.h"
#include "token.h"
#include "util.h"

enum state { hdr, hdr_rel, bdy, end };

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
            if (!line_hdr_cont(l))
                return hdr;
            break;

        case end:
            break;
    }
    return s;
}

void maybe_save(enum state old, enum state cur,
        struct line *t, struct line *x) {
    char *p;
    enum { no, raw, cooked } save = no;
    struct line tmp = { 0 };

    switch (cur) {
        case hdr_rel:
            line_copy(&tmp, x);
            if ((p = memchr(tmp.x, ':', tmp.l) + 1)) {
                    memmove(tmp.x, p, tmp.x + tmp.l - p);
                    tmp.l -= p - tmp.x;
            }
            save = cooked;
            break;
    }
    if (save == no)
        return;

    line_cat(t, save == raw ? x : &tmp);
    assert(t->l > 0);
    --t->l;
}

void maybe_submit(enum state old, enum state cur, struct line *t) {
    _Bool submit = 1;

    if (t->l == 0)
        return;
    switch (old) {
        case hdr:
        case hdr_rel:
            break;
    }
    if (!submit)
        return;
    tokenize(t->x, t->l, 0);
    t->l = 0;
}

_Bool read_email(const _Bool fromline, FILE *in, FILE **tmp) {
    enum state s_old, s_cur;
    /* One line to Tokenize, and one to Xmit */
    struct line t = { 0 }, x = { 0 };

    s_old = hdr;

    /* If tmp is set, we are in "passthrough" mode; *tmp is the FILE * where we
     * will stash the body of the email, to be output later. */
    if (tmp && !(*tmp = tmpfile()))
            return 0;

    /* The state machine. We read a line, which may move us to a new state.
     * Depending on the transition we just performed, we may submit the held
     * text. If the new state is end, that's all folks. Depending on the
     * transition we just performed, we may hold the line (or part of it). If
     * we are in pass through mode, write the line, either to stdout or the
     * save file. */
    while (1) {
        line_read(in, &x);
        nbytesrd += x.l;
        s_cur = transition(s_old, &x);
        maybe_submit(s_old, s_cur, &t);
        if (s_cur == end) break;
        maybe_save(s_old, s_cur, &t, &x);
        if (tmp && !line_write(stdout, &x))
            goto abort;
        s_old = s_cur;
    }
    return 1;

abort:
    if (*tmp)
        fclose(*tmp);
    return 0;
}
