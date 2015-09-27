/*
 * bfilter.c:
 * Simple Bayesian email filter, in C.
 *
 * Copyright (c) 2003 Chris Lightfoot. All rights reserved.
 * Email: chris@ex-parrot.com; WWW: http://www.ex-parrot.com/~chris/
 *
 */

#include <sys/types.h>

#include <errno.h>
#include <math.h>
#include <pwd.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <unistd.h>

#include <netinet/in.h>     /* for ntohl/htonl */

#include "bayes.h"
#include "bfilter.h"
#include "compose.h"
#include "db.h"
#include "settings.h"
#include "skiplist.h"
#include "submit.h"
#include "util.h"

/* usage STREAM
 * Print a usage message to STREAM. */
void usage(FILE *stream) {
    fprintf(stream,
"bfilter [FLAGS] COMMAND\n"
"\n"
"  Commands:\n"
"    isspam      From_ separated emails are read, added to spam corpus\n"
"    isreal                                                real\n"
"    test        An X-Spam-Probability header is added to the email\n"
"                read from standard input\n"
"    cleandb     Discard little-used terms from the database.\n"
"    stats       Print some statistics about the database.\n"
"\n"
"  Flags:\n"
"    -b          Treat input as Berkeley format mbox file\n"
"\n"
"bfilter, version " BFILTER_VERSION "\n"
"Copyright (c) 2003-4 Chris Lightfoot <chris@ex-parrot.com>\n"
"Copyright (c) 2015 Toby Goodwin <toby@paccrat.org>\n"
"https://github.com/TobyGoodwin/bfilter\n"
        );
}

int flagb = 0;

/* main ARGC ARGV
 * Entry point. Usage:
 *
 *  bfilter isspam      From_ separated emails are read, added to spam corpus
 *          isreal                                                real
 *          test        An X-Spam-Probability header is added to the email
 *                      read from standard input
 *          cleandb     Discard terms which have not been used recently.
 *          stats       Print some statistics about the database.
 */
int main(int argc, char *argv[]) {
    enum { isspam, isreal, test, cleandb, stats } mode;
    skiplist_iterator si;
    FILE *tempfile;
    int retval = 0;
    int arg = 1;

    while (argv[arg] && *argv[arg] == '-') {
        char *cp = argv[arg] + 1;
        while (*cp) {
            switch (*cp) {
                case 'b':
                    flagb = 1;
                    break;
                default:
                    usage(stderr);
                    return 1;
            }
            ++cp;
        }
        ++arg;
    }

    if (argc - arg != 1) {
        usage(stderr);
        return 1;
    } else if (strcmp(argv[arg], "isspam") == 0)
        mode = isspam;
    else if (strcmp(argv[arg], "isreal") == 0)
        mode = isreal;
    else if (strcmp(argv[arg], "test") == 0)
        mode = test;
    else if (strcmp(argv[arg], "cleandb") == 0)
        mode = cleandb;
    else if (strcmp(argv[arg], "stats") == 0)
        mode = stats;
    else {
        usage(stderr);
        return 1;
    }

    /* Now read whatever emails we need to, and record the terms which appear
     * in them. */
    wordlist = skiplist_new(NULL);

    switch (mode) {
        case isspam:
        case isreal:
            do {
                int f;
                errno = 0;
                ++nemails;
                f = read_email(flagb, 0, stdin, NULL);
                if (!f) {
                    fprintf(stderr, "bfilter: error while reading email (%s)\n", errno ? strerror(errno) : "no system error");
                    return 1;
                }
             
                /* If we're running on a terminal, print stats. */
                if (isatty(1))
                    fprintf(stderr, "Reading: %8u emails (%8u bytes) %8lu terms avg length %8.2f\r", nemails, (unsigned)nbytesrd, skiplist_size(wordlist), (double)term_length / skiplist_size(wordlist));
            } while (!feof(stdin));
            if (isatty(1))
                fprintf(stderr, "\n");
            break;

        case test:
            /* Read a single email in pass-through mode. */
            errno = 0;
            if (!read_email(0, 1, stdin, &tempfile)) {
                fprintf(stderr, "bfilter: failed to read email (%s)\n", errno ? strerror(errno) : "no system error");
                return 1;
            }
            break;

        case cleandb:
        case stats:
            break;
    }

    if (!db_open())
        return 1;
    
    if (mode == isspam || mode == isreal) {
        /* Update total number of emails and the data for each word. */
        int nspam, nreal;
        char *term = NULL;
        size_t termlen = 1;
        unsigned int nterms, ntermswr, ntermsnew;
        
        if (!db_get_pair("__emails__", &nspam, &nreal))
            nspam = nreal = 0;
        if (mode == isspam)
            nspam += nemails;
        else
            nreal += nemails;
        db_set_pair("__emails__", nspam, nreal);

        if (isatty(1))
            fprintf(stderr, "Writing: corpus now contains %8u spam / %8u nonspam emails\n", nspam, nreal);

        nterms = skiplist_size(wordlist);
        
        for (si = skiplist_itr_first(wordlist), ntermswr = 0, ntermsnew = 0; si; si = skiplist_itr_next(wordlist, si), ++ntermswr) {
            char *k;
            size_t kl;
            struct wordcount *pw;
            
            k = skiplist_itr_key(wordlist, si, &kl);
            
            if (!term || kl + 1 > termlen)
                term = xrealloc(term, termlen = 2 * (kl + 1));
            term[kl] = 0;
            memcpy(term, k, kl);
            
            pw = skiplist_itr_value(wordlist, si);

            if (!db_get_pair(term, &nspam, &nreal)) {
                nspam = nreal = 0;
                ++ntermsnew;
            }
            
            if (mode == isspam)
                nspam += pw->n;
            else
                nreal += pw->n;

            db_set_pair(term, nspam, nreal);

            if (isatty(1) && (ntermswr % 500) == 0)
                fprintf(stderr, "Writing: %8u / %8u terms (%8u new)\r", ntermswr, nterms, ntermsnew);
        }

        if (isatty(1))
            fprintf(stderr, "Writing: %8u / %8u terms (%8u new)\n", ntermswr, nterms, ntermsnew);

        free(term);
    } else if (mode == test) {
        double score;
        /* The headers of the email have already been written to standard
         * output; we compute a `spam probability' from the words we've read
         * and those recorded in the database, write out an appropriate
         * header and then dump the rest of the email. */
        score = bayes(wordlist);
#if 0
        int nspamtotal, nrealtotal;
        skiplist problist;
        size_t nterms, n, nsig = 15;
        float a = 1., b = 1., loga = 0., logb = 0., score, logscore;
        
        problist = skiplist_new(compare_by_probability);
        db_get_pair("__emails__", &nspamtotal, &nrealtotal);
        
        for (si = skiplist_itr_first(wordlist); si; si = skiplist_itr_next(wordlist, si)) {
            struct termprob t = { 0 };
            int nspam, nreal;
            
            t.term = (char*)skiplist_itr_key(wordlist, si, &t.tlen);
            t.prob = 0.4;

            if (db_get_pair(t.term, &nspam, &nreal))
                t.prob = ((float)nspam / (float)nspamtotal) / ((float)nspam / (float)nspamtotal + (float)nreal / (float)nrealtotal);

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

#endif
        printf("X-Spam-Probability: %s (p=%f)\n",
                score > SPAM_THRESHOLD ? "YES" : "NO", score);
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
    } else if (mode == cleandb)
        /* Copy recent data to new database, replace old one. */
        db_clean(28);
    else if (mode == stats)
        /* Print some statistics about the database. */
        db_print_stats();

    db_close();

    skiplist_delete(wordlist);

    return retval;
}

