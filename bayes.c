#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "bayes.h"
#include "bfilter.h"
#include "db.h"
#include "submit.h"

void skiplist_dump(skiplist s) {
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
    float p1, p2;
    t1 = (struct termprob*)k1;
    t2 = (struct termprob*)k2;
    p1 = fabs(0.5 - t1->prob);
    p2 = fabs(0.5 - t2->prob);
    if (p1 < p2)
        return 1;
    else if (p1 > p2)
        return -1;
    else {
        if (t1->tlen < t2->tlen)
            return 1;
        else if (t1->tlen > t2->tlen)
            return -1;
        else
            return memcmp(t1->term, t2->term, t1->tlen);
    }
}

/* this needs to be better refactored */
int bayes(skiplist wordlist, FILE *tempfile) {
    /* The headers of the email have already been written to standard
     * output; we compute a `spam probability' from the words we've read
     * and those recorded in the database, write out an appropriate
     * header and then dump the rest of the email. */
    int nspamtotal, nrealtotal;
    int retval;
    skiplist problist;
    skiplist_iterator si;
    size_t nterms, n, nsig = 15;
    float a = 1., b = 1., loga = 0., logb = 0., score, logscore;

    //skiplist_dump(wordlist);
    problist = skiplist_new(compare_by_probability);
    db_get_pair("__emails__", &nspamtotal, &nrealtotal);

    for (si = skiplist_itr_first(wordlist); si; si = skiplist_itr_next(wordlist, si)) {
        struct termprob t = { 0 };
        int nspam, nreal;

        t.term = (char*)skiplist_itr_key(wordlist, si, &t.tlen);
        t.prob = 0.4;

        if (db_get_pair(t.term, &nspam, &nreal)) {
            nreal *= 2;
            if (nreal + nspam > 3)
                t.prob = ((float)nspam / (float)nspamtotal) / ((float)nspam / (float)nspamtotal + (float)nreal / (float)nrealtotal);
        }

fprintf(stderr, "%.*s => %f (%d, %d)\n", (int)t.tlen, t.term, t.prob, nreal, nspam);
        if (t.prob == 0.)
            t.prob = 0.00001;
        else if (t.prob == 1.)
            t.prob = 0.99999;

        skiplist_insert(problist, &t, sizeof t, NULL); /* shouldn't fail */
    }

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
            printf(" (%6.4f)", tp->prob);
        }
        a *= tp->prob;
        loga += log(tp->prob);
        b *= 1. - tp->prob;
        logb += log(1. - tp->prob);
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
