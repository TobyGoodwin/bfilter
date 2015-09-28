/*
 * bfilter.c:
 * Simple Bayesian email filter, in C.
 *
 * Copyright (c) 2003 Chris Lightfoot. All rights reserved.
 * Copyright (c) 2015 Toby Goodwin.
 * toby@paccrat.org
 * https://github.com/TobyGoodwin/bfilter
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
"    isspam      Train email as spam\n"
"    isreal      Train email as real\n"
"    test        Writes p(spam) for the email read from standard input\n"
"    annotate    An X-Spam-Probability header is added to the email\n"
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

_Bool fdump(FILE *f) {
    rewind(f);
    do {
        unsigned char buf[8192];
        size_t n;

        n = fread(buf, 1, 8192, f);
        if (ferror(f) || (n > 0 && fwrite(buf, 1, n, stdout) != n))
            break;
    } while (!feof(f) && !ferror(f));

    if (ferror(f)) {
        fprintf(stderr, "bfilter: temporary file: read error (%s)\n",
                strerror(errno));
        return 0;
    }

    fflush(stdout);
    if (ferror(stdout)) {
        fprintf(stderr, "bfilter: standard output: write error (%s)\n",
                strerror(errno));
        return 0;
    }

    return 1;
}

enum mode { isspam, isreal, test, annotate, cleandb, stats } mode;
int run(enum mode mode);

/* main ARGC ARGV
 * Entry point. Usage:
 *
 *  bfilter isspam      From_ separated emails are read, added to spam corpus
 *          isreal                                                real
 *          test        read from standard input, write p(spam)
 *          annotate    An X-Spam-Probability header is added to the email
 *                      read from standard input
 *          cleandb     Discard terms which have not been used recently.
 *          stats       Print some statistics about the database.
 */
int main(int argc, char *argv[]) {
    enum mode mode;
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
    else if (strcmp(argv[arg], "annotate") == 0)
        mode = annotate;
    else if (strcmp(argv[arg], "cleandb") == 0)
        mode = cleandb;
    else if (strcmp(argv[arg], "stats") == 0)
        mode = stats;
    else {
        usage(stderr);
        return 1;
    }

    return run(mode);
}

_Bool train_read(void) {
    do {
        int f;
        errno = 0;
        ++nemails;
        f = read_email(flagb, 0, stdin, NULL);
        if (!f) {
            fprintf(stderr, "bfilter: error while reading email (%s)\n",
                    errno ? strerror(errno) : "no system error");
            return 0;
        }

        /* If we're running on a terminal, print stats. */
        if (isatty(1))
            fprintf(stderr,
        "Reading: %8u emails (%8u bytes) %8lu terms avg length %8.2f\r",
                nemails, (unsigned)nbytesrd, skiplist_size(token_list),
                (double)term_length / skiplist_size(token_list));
    } while (!feof(stdin));
    if (isatty(1))
        fprintf(stderr, "\n");
    return 1;
}

void train_update(enum mode mode) {
    /* Update total number of emails and the data for each word. */
    int nspam, nreal;
    char *term = NULL;
    size_t termlen = 1;
    skiplist_iterator si;
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

    nterms = skiplist_size(token_list);

    for (si = skiplist_itr_first(token_list), ntermswr = 0, ntermsnew = 0; si; si = skiplist_itr_next(token_list, si), ++ntermswr) {
        char *k;
        size_t kl;
        struct wordcount *pw;

        k = skiplist_itr_key(token_list, si, &kl);

        if (!term || kl + 1 > termlen)
            term = xrealloc(term, termlen = 2 * (kl + 1));
        term[kl] = 0;
        memcpy(term, k, kl);

        pw = skiplist_itr_value(token_list, si);

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
}

int run(enum mode mode) {
    int retval;
    FILE *tempfile;

    /* Now read whatever emails we need to, and record the terms which appear
     * in them. */
    token_list = skiplist_new(NULL);

    switch (mode) {
        case isspam:
        case isreal:
            if (!train_read())
                return 1;
            break;

        case test:
            /* Read a single email. */
            errno = 0;
            if (!read_email(0, 0, stdin, 0)) {
                fprintf(stderr, "bfilter: failed to read email (%s)\n",
                        errno ? strerror(errno) : "no system error");
                return 1;
            }
            break;

        case annotate:
            /* Read a single email in pass-through mode. */
            errno = 0;
            if (!read_email(0, 1, stdin, &tempfile)) {
                fprintf(stderr, "bfilter: failed to read email (%s)\n",
                        errno ? strerror(errno) : "no system error");
                return 1;
            }
            break;

        case cleandb:
        case stats:
            break;
    }

    if (!db_open())
        return 1;
    
    switch (mode) {
        case isspam:
        case isreal:
            train_update(mode);
            break;

        case test:
            printf("%f\n", bayes(token_list));
            break;

        case annotate:
            {
                double score;
                /* Headers of the email have already been written, and
                 * remainder saved in tempfile. Compute p(spam), write
                 * our header, then dump the rest of the email. */
                score = bayes(token_list);
                printf("X-Spam-Probability: %s (p=%f)\n",
                        score > SPAM_THRESHOLD ? "YES" : "NO", score);
                if (!fdump(tempfile)) retval = 1;
            }
            break;

        case cleandb:
            /* Copy recent data to new database, replace old one. */
            db_clean(28);
            break;

        case stats:
            db_print_stats();
            break;
    }

    db_close();

    skiplist_delete(token_list);

    return retval;
}
