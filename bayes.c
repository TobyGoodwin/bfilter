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
#define UNSURE ((uint8_t *)"UNSURE")

uint8_t *bayes(skiplist tokens) {
    struct class *class, *classes;
    size_t nc;
    int i, n_total;
    double score[20]; /* XXX */
    double maxprob = -DBL_MAX, minprob = 0.;
    uint8_t *maxclass;
    uint32_t *p_ui32, t_total;
   
    classes = class_fetch();
    if (classes->code == 0)
        return UNSURE;
    p_ui32 = db_hash_fetch_uint32((uint8_t *)EMAILS_KEY, sizeof EMAILS_KEY - 1);
    if (p_ui32) n_total = *p_ui32;
    else return UNSURE;
    n_total = *db_hash_fetch((uint8_t *)EMAILS_KEY, sizeof EMAILS_KEY - 1, &nc);
    p_ui32 = db_hash_fetch_uint32((uint8_t *)VOCAB_KEY, sizeof VOCAB_KEY - 1);
    if (p_ui32) t_total = *p_ui32;
    else return UNSURE;
    TRACE fprintf(stderr, "documents (emails trained): %d\n", n_total);
    TRACE fprintf(stderr, "vocabulary (total distinct terms): %d\n", t_total);

    for (class = classes; class->code; ++class) {
        skiplist_iterator si;

        TRACE fprintf(stderr, "class %s (%d)\n", class->name, class->code);
        TRACE fprintf(stderr, "emails in class %s: %d\n",
                class->name, class->docs);
        TRACE fprintf(stderr, "prior(%s): %f\n",
                class->name, (double)class->docs / n_total);
        score[class->code] = log((double)class->docs / (double)n_total);

        TRACE fprintf(stderr, "t_%s = %d, t_total = %d\n",
                class->name, class->terms, t_total);
        TRACE fprintf(stderr, "terms in class %s: %d\n",
                class->name, class->terms);
        for (si = skiplist_itr_first(tokens); si;
                si = skiplist_itr_next(tokens, si)) {
            double p;
            uint8_t *t;
            size_t t_len;
            uint32_t *cnts;
            int ncnts, Tct;
            int occurs; /* number of occurences of this term in test text */

            t = skiplist_itr_key(tokens, si, &t_len);
            occurs = *(int *)skiplist_itr_value(tokens, si);
            TRACE fprintf(stderr, "term %.*s occurs %d times\n",
                    (int)t_len, t, occurs);
            cnts = db_get_intlist(t, t_len, &ncnts);
            if (!cnts) continue; /* not in training vocabulary */
            Tct = 0;
            for (i = 0; i < ncnts; i += 2)
                if (cnts[i] == class->code) {
                    Tct = cnts[i + 1];
                    break;
                }
            TRACE fprintf(stderr, "Tct = %d\n", Tct);
            p = (Tct + 1.) / (class->terms + t_total);
            TRACE fprintf(stderr, "condprob[%s][%.*s] = %g\n",
                    class->name, (int)t_len, t, p);
            score[class->code] += occurs * log(p);
        }
    }

    /* check the range of logprobs, and return UNSURE if it is too small (XXX
     * would it be better just to compare the distance between the winner and
     * the runner up?) */
    for (class = classes; class->code; ++class) {
        TRACE fprintf(stderr, "score(%s): %f\n",
                class->name, score[class->code]);
        if (score[class->code] < minprob) {
            minprob = score[class->code];
        }
        if (score[class->code] > maxprob) {
            maxprob = score[class->code];
            maxclass = class->name;
        }
    }
    TRACE fprintf(stderr, "logprob range: %f\n", maxprob - minprob);
    if (maxprob - minprob < UNSURE_LOG_RANGE)
        return UNSURE;

    return maxclass;
}



