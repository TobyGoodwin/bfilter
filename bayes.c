#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "bayes.h"
#include "bfilter.h"
#include "db.h"
#include "submit.h"

struct termprob {
    double p_spam;
    double p_present;
    char *term;
    size_t tlen;
};

double termprob_radius(struct termprob *tp) {
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

static void wordlist_dump(skiplist s) {
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

static int compare_by_probability(const void *k1, const size_t k1len,
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

/* this needs to be better refactored */
int bayes(skiplist wordlist, FILE *tempfile) {
    /* The headers of the email have already been written to standard
     * output; we compute a `spam probability' from the words we've read
     * and those recorded in the database, write out an appropriate
     * header and then dump the rest of the email. */
    int inspamtotal, inrealtotal;
    double nspamtotal, nrealtotal;
    int retval;
    skiplist problist;
    skiplist_iterator si;
    size_t nterms, n, nsig = 15;
    double a = 1., b = 1., loga = 0., logb = 0., score, logscore;

    //skiplist_dump(wordlist);
    problist = skiplist_new(compare_by_probability);
    db_get_pair("__emails__", &inspamtotal, &inrealtotal);
    nspamtotal = inspamtotal; nrealtotal = inrealtotal;

    for (si = skiplist_itr_first(wordlist); si; si = skiplist_itr_next(wordlist, si)) {
        struct termprob t = { 0 };
        int inspam, inreal;

        t.term = (char*)skiplist_itr_key(wordlist, si, &t.tlen);
        t.p_spam = 0.4;

        if (db_get_pair(t.term, &inspam, &inreal)) {
            double nspam = inspam, nreal = inreal;
//            nreal *= 2;
//            if (nreal + nspam > 3)
            t.p_spam = (nspam / nspamtotal) /
                (nspam / nspamtotal + nreal / nrealtotal);
//fprintf(stderr, "%.*s => %f (%d, %d)\n", (int)t.tlen, t.term, t.prob, nreal, nspam);
            t.p_present = (nspam + nreal) / (nspamtotal + nrealtotal);
        }

        if (t.p_spam < 0.01)
            t.p_spam = 0.01;
        else if (t.p_spam > 0.99)
            t.p_spam = 0.99;

        skiplist_insert(problist, &t, sizeof t, NULL); /* shouldn't fail */
    }

problist_dump(problist);
    nterms = skiplist_size(problist);
    printf("X-Spam-Words: %lu terms\n significant:", nterms);
    if (nsig > nterms)
        nsig = nterms;

    for (si = skiplist_itr_first(problist), n = 0; si && n < nsig; si = skiplist_itr_next(problist, si), ++n) {
        struct termprob *tp;
        tp = (struct termprob*)skiplist_itr_key(problist, si, NULL);

//fprintf(stderr, "considering %.*s => %f\n", (int)tp->tlen, tp->term, tp->prob);
        if (n < 6) {
            /* Avoid emitting high bit characters in the header. */
            char *p;
            putchar(' ');
            for (p = tp->term; *p; ++p)
                if (*p < 0x80)
                    putchar(*p);
                else
                    printf("\\x%02x", (unsigned int)*p);
            printf(" (%6.4f)", tp->p_spam);
        }
        a *= tp->p_spam;
        loga += log(tp->p_spam);
        b *= 1. - tp->p_spam;
        logb += log(1. - tp->p_spam);
    }

    printf("\n");

    logscore = loga - log(exp(loga) + exp(logb));

    if (a == 0.)
        score = 0.;
    else
        score = a / (a + b);

    printf("X-Spam-Probability: %s (p=%f, |log p|=%f)\n", score > 0.9 ? "YES" : "NO", score, fabs(logscore));

    fseek(tempfile, 0, SEEK_SET);
    do {
        unsigned char buf[8192];
        size_t n;

        n = fread(buf, 1, 8192, tempfile);
        if (ferror(tempfile) || (n > 0 && fwrite(buf, 1, n, stdout) != n))
            break;
    } while (!feof(tempfile) && !ferror(tempfile));

    if (ferror(tempfile)) {
        fprintf(stderr, "bfilter: temporary file: read error (%s)\n", strerror(errno));
        retval = 1;
    } else if (ferror(stdout)) {
        fprintf(stderr, "bfilter: standard output: write error (%s)\n", strerror(errno));
        retval = 1;
    }

    return retval;
}
