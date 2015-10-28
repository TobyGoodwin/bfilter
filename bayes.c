#include <assert.h>
#include <errno.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "bayes.h"
#include "bfilter.h"
#include "count.h"
#include "db.h"
#include "settings.h"

#define TRACE if (0)

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

uint8_t *bayes(skiplist tokens) {
    struct class *class, *classes;
    uint32_t *epc, *tpc;
    int i, nc, nt, n_class, n_total, t_class, t_total;
    double score[20]; /* XXX */
    double maxprob = -DBL_MAX, minprob = 0.;
    uint8_t *maxclass;
   
    classes = db_get_classes();
    /* XXX */
    n_total = *db_get_intlist((uint8_t *)EMAILS_KEY, sizeof EMAILS_KEY - 1, &nc);
    t_total = *db_get_intlist((uint8_t *)VOCAB_KEY, sizeof VOCAB_KEY - 1, &nc);
    TRACE fprintf(stderr, "total terms: %d\n", t_total);
    epc = db_get_intlist((uint8_t *)EMAILS_CLASS_KEY, sizeof EMAILS_CLASS_KEY - 1, &nc);

    tpc = db_get_intlist((uint8_t *)TERMS_CLASS_KEY, sizeof TERMS_CLASS_KEY - 1, &nt);

    for (class = classes; class->code; ++class) {
        skiplist_iterator si;

        TRACE fprintf(stderr, "class %s (%d)\n", class->name, class->code);
        for (i = 0; i < nc; i += 2)
            if (epc[i] == class->code) {
                n_class = epc[i + 1];
                break;
            }
        /* what happens if this class isn't in emails per class? redesign so it
         * can't happen */
        assert(i < nc);
        TRACE fprintf(stderr, "emails in class %s: %d\n", class->name, n_class);
        TRACE fprintf(stderr, "prior(%s): %f\n", class->name, (double)n_class / n_total);
        score[class->code] = log((double)n_class / n_total);

        for (i = 0; i < nt; i += 2) {
            if (tpc[i] == class->code) {
                t_class = tpc[i + 1];
                break;
            }
        }
        TRACE fprintf(stderr, "t_%s = %d, t_total = %d\n", class->name, t_class, t_total);
        // t_class = t_total; /* just to see */
        /* what happens if this class isn't in terms per class? redesign so it
         * can't happen */
        assert(i < nt);
        TRACE fprintf(stderr, "terms in class %s: %d\n", class->name, t_class);
        for (si = skiplist_itr_first(tokens); si;
                si = skiplist_itr_next(tokens, si)) {
            double p;
            struct termprob t = { 0 };
            uint32_t *cnts;
            int ncnts, Tct;
            int occurs; /* number of occurences of this term in test text */

            t.term = (char*)skiplist_itr_key(tokens, si, &t.tlen);
            occurs = *(int *)skiplist_itr_value(tokens, si);
            TRACE fprintf(stderr, "term %.*s occurs %d times\n", (int)t.tlen, t.term, occurs);

            for (i = 0; i < nc; i += 2)
                if (epc[i] == class->code) {
                    n_class = epc[i + 1];
                    break;
                }
            cnts = db_get_intlist(t.term, t.tlen, &ncnts);
            if (!cnts) continue; /* not in training vocabulary */
            Tct = 0;
            for (i = 0; i < ncnts; i += 2)
                if (cnts[i] == class->code) {
                    Tct = cnts[i + 1];
                    break;
                }
            TRACE fprintf(stderr, "Tct = %d\n", Tct);
            p = (Tct + 1.) / (t_class + t_total);
            TRACE fprintf(stderr, "condprob[%s][%.*s] = %g\n", class->name, (int) t.tlen, t.term, p);
            score[class->code] += occurs * log(p);
        }

    }

    for (class = classes; class->code; ++class) {
        TRACE fprintf(stderr, "score(%s): %f\n", class->name, score[class->code]);
        if (score[class->code] < minprob) {
            minprob = score[class->code];
        }
        if (score[class->code] > maxprob) {
            maxprob = score[class->code];
            maxclass = class->name;
        }
    }
    TRACE fprintf(stderr, "logprob range: %f\n", maxprob - minprob);
    if (maxprob - minprob < 3.)
        return (uint8_t *)"UNSURE";

    return maxclass;
}


double bayes_old(skiplist tokens) {
    int inspamtotal, inrealtotal;
    double nspamtotal, nrealtotal;
    /* XXX use a heap for this - although asymptotically it may be the same
     * order as a skiplist, the constants should be rather smaller for both
     * space and time. */
    skiplist problist;
    skiplist_iterator si;
    size_t nterms, n, nsig = SIGNIFICANT_TERMS;
    double a = 1., b = 1.;

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

    if (flagD && strchr(flagD, 'p')) problist_dump(problist);
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
