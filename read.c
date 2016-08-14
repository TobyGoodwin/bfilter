#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

#include "bfilter.h"
#include "cook.h"
#include "line.h"
#include "read.h"
#include "token.h"
#include "util.h"

#define TRACE if (0)

/* note that hdr_cont at the moment really means hdr_rel_cont */
enum state { hdr, hdr_rel, hdr_cont, hdr_mine,
             blank,
             bdy, bdy_b64, bdy_soft_eol,
             end };

enum state transition(_Bool fromline, enum state s, struct line *l) {
    if (line_empty(l))
        return end;
    assert(l->x[l->l - 1] == '\n');
    if (l->l == 1)
        return blank;
    switch (s) {
        case blank:
            if (fromline && line_starts(l, "From "))
                return end;
            if (line_is_b64(l))
                return bdy_b64;
            if (line_ends(l, "=\n"))
                return bdy_soft_eol;
            return bdy;

        case bdy:
            if (line_ends(l, "=\n"))
                return bdy_soft_eol;
            break;

        /* can't transition from b64 to soft_eol or vice versa */
        case bdy_b64:
            if (!line_is_b64(l))
                return bdy;
            break;

        case bdy_soft_eol:
            if (!line_ends(l, "=\n"))
                return bdy;
            break;

        case hdr_mine:
        case hdr_rel:
        case hdr_cont:
            if (line_hdr_cont(l))
                return s == hdr_mine ? s : hdr_cont;

            /* fall through */

        case hdr:
            if (
                    line_starts_ci(l, "from:") ||
                    line_starts_ci(l, "return-path:") ||
                    line_starts_ci(l, "subject:") ||
                    line_starts_ci(l, "to:") ||
                    line_starts_ci(l, "x-spam-status:")
                ) return hdr_rel;
            if (line_starts_ci(l, "x-spam-probability:"))
                return hdr_mine;
            return hdr;

        case end:
            break;
    }

    return s;
}

void maybe_save(enum state old, enum state cur,
        struct line *t, struct line *x) {

    switch (cur) {
        case hdr:
        case hdr_mine:
        case blank:
            return;

        default:
            break;
    }

    line_cat(t, x);
    /* drop trailing \n */
    assert(t->l > 0);
    --t->l;

    if (cur == bdy_soft_eol) {
        assert(line_ends(t, "="));
        --t->l;
    }
}

void maybe_submit(enum state old, enum state cur, struct line *t) {
    _Bool submit = 0;

    if (t->l == 0)
        return;
    switch (old) {
        case bdy:
            submit = 1;
            cook_qp(t);
            cook_entities(t);
            break;

        case bdy_soft_eol:
            if (cur != bdy_soft_eol && cur != bdy) {
                submit = 1;
                cook_qp(t);
                cook_entities(t);
            }
            break;

        case bdy_b64:
            if (cur != bdy_b64) {
                cook_b64(t);
                if (is_text(t))
                    submit = 1;
                else
                    t->l = 0;
            }
            break;

        case hdr_rel:
        case hdr_cont:
            if (cur != hdr_cont) {
                submit = 1;
                cook_header(t);
            }
            break;
            
        default:
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
        TRACE fprintf(stderr, "read %zu (%.*s)\n", x.l,
                x.l < 40 ? (int)x.l - 1 : 40, x.x);
        nbytesrd += x.l;
        s_cur = transition(fromline, s_old, &x);
        TRACE fprintf(stderr, "state %d => %d\n", s_old, s_cur);
        maybe_submit(s_old, s_cur, &t);
        if (s_cur == end) break;
        maybe_save(s_old, s_cur, &t, &x);
        if (tmp && s_cur != hdr_mine)
           if (!line_write(stdout, &x))
               goto abort;
        s_old = s_cur;
    }
    /* Ugh. If we're in Berkeley mbox mode, we've already read the "From " at
     * the top of the next message, so write it out if in passthrough mode. */
    if (tmp && fromline && x.l > 0)
        if (!line_write(stdout, &x))
            goto abort;
    return 1;

abort:
    if (*tmp)
        fclose(*tmp);
    return 0;
}
