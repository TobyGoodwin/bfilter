#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "bayes.h"
#include "db.h"
#include "settings.h"
/* for struct wordcount: */
#include "submit.h"

struct termprob {
    double p_spam;
    double p_present;
    char *term;
    size_t tlen;
};

/* XXX don't actually need to take the root here */
static double termprob_radius(struct termprob *tp) {
    double x = tp->p_spam * 2 - 1;
    double y = tp->p_present;

    return sqrt(x * x + y * y);
}

static void problist_dump(skiplist s) {
    skiplist_iterator x;

    for (x = skiplist_itr_first(s); x; x =skiplist_itr_next(s, x)) {
        struct termprob *tp;

        tp = skiplist_itr_key(s, x, 0);
        printf("%.*s => %f, %f => %f\n", (int)tp->tlen, tp->term,
                tp->p_spam, tp->p_present, termprob_radius(tp));
    }
}

static void token_list_dump(skiplist s) {
    skiplist_iterator x;

    for (x = skiplist_itr_first(s); x; x =skiplist_itr_next(s, x)) {
        char *k;
        struct wordcount *pw;
        size_t l;

        k = (char *)skiplist_itr_key(s, x, &l);
        pw = skiplist_itr_value(s, x);
        printf("%.*s => (%d, %d)\n", (int)l, k, pw->nemail, pw->n);
    }
}

static int termprob_compare(const void *k1, const size_t k1len,
        const void *k2, const size_t k2len) {
    struct termprob *t1, *t2;
    double r1, r2;

    t1 = (struct termprob*)k1;
    t2 = (struct termprob*)k2;

    r1 = termprob_radius(t1);
    r2 = termprob_radius(t2);

    if (fabs(r1 - r2) > 0.001) {
        if (r1 < r2)
            return 1;
        if (r1 > r2)
            return -1;
    }
    if (t1->tlen < t2->tlen)
        return 1;
    if (t1->tlen > t2->tlen)
        return -1;
    return memcmp(t1->term, t2->term, t1->tlen);
}

/* Calculate an overall spam probability for a token list */
double bayes(skiplist tokens) {
    int inspamtotal, inrealtotal;
    double nspamtotal, nrealtotal;
    /* XXX use a heap for this - although asymptotically it may be the same
     * order as a skiplist, the constants should be rather smaller for both
     * space and time. */
    skiplist problist;
    skiplist_iterator si;
    size_t nterms, n, nsig = SIGNIFICANT_TERMS;
    double a = 1., b = 1.;

//token_list_dump(tokens);
    
    problist = skiplist_new(termprob_compare);
    db_get_pair("__emails__", &inspamtotal, &inrealtotal);
    if (inspamtotal == 0 || inrealtotal == 0)
        return 0.;
    nspamtotal = inspamtotal; nrealtotal = inrealtotal;

    for (si = skiplist_itr_first(tokens); si;
            si = skiplist_itr_next(tokens, si)) {
        struct termprob t = { 0 };
        int inspam, inreal;

        t.term = (char*)skiplist_itr_key(tokens, si, &t.tlen);
        t.p_spam = 0.4;

        if (db_get_pair(t.term, &inspam, &inreal)) {
            double nspam = inspam, nreal = inreal;
            t.p_spam = (nspam / nspamtotal) /
                (nspam / nspamtotal + nreal / nrealtotal);
            t.p_present = (nspam + nreal) / (nspamtotal + nrealtotal);
        }

        if (t.p_spam < 0.01) t.p_spam = 0.01;
        if (t.p_spam > 0.99) t.p_spam = 0.99;

        skiplist_insert(problist, &t, sizeof t, NULL); /* shouldn't fail */
    }

//problist_dump(problist);
    nterms = skiplist_size(problist);
    if (nsig > nterms) nsig = nterms;

    for (si = skiplist_itr_first(problist), n = 0;
            si && n < nsig; si = skiplist_itr_next(problist, si), ++n) {
        struct termprob *tp;

        tp = (struct termprob*)skiplist_itr_key(problist, si, NULL);
//printf("%.*s => %f, %f => %f\n", (int)tp->tlen, tp->term, tp->p_spam, tp->p_present, termprob_radius(tp));
        a *= tp->p_spam;
        b *= 1. - tp->p_spam;
    }

    skiplist_delete(problist);

    return a / (a + b);
}
