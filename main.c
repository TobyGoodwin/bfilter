/*

    Copyright (c) 2003 Chris Lightfoot. All rights reserved.
    Copyright (c) 2015 Toby Goodwin.
    toby@paccrat.org
    https://github.com/TobyGoodwin/bfilter

    This file is part of bfilter.

    Bfilter is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Bfilter is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with bfilter.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "bayes.h"
#include "bfilter.h"
#include "db.h"
#include "fdump.h"
#include "read.h"
#include "settings.h"
#include "submit.h"
#include "train.h"

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

_Bool flagb = 0;
char *flagD = 0;


int run(enum mode mode, char *cclass);

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
    char *cclass = 0;
    enum mode mode = error;
    int arg = 1;

    while (argv[arg] && *argv[arg] == '-') {
        char *cp = argv[arg] + 1;
        while (*cp) {
            switch (*cp) {
                case 'b': flagb = 1; break;
                case 'D': flagD = cp; goto next_arg;
                default: usage(stderr); return 1;
            }
            ++cp;
        }
next_arg:
        ++arg;
    }

    switch (argc - arg) {
        case 1:
            if (strcmp(argv[arg], "isspam") == 0) {
                mode = train;
                cclass = "spam";
            } else if (strcmp(argv[arg], "isreal") == 0) {
                mode = train;
                cclass = "real";
            } else if (strcmp(argv[arg], "test") == 0)
                mode = test;
            else if (strcmp(argv[arg], "annotate") == 0)
                mode = annotate;
            else if (strcmp(argv[arg], "cleandb") == 0)
                mode = cleandb;
            else if (strcmp(argv[arg], "stats") == 0)
                mode = stats;
            break;

        case 2:
            if (strcmp(argv[arg], "train") == 0) {
                mode = train;
                cclass = argv[arg + 1];
            }
            break;

        default:
            mode = error;
            break;
    }

    return run(mode, cclass);
}

static void token_list_dump(skiplist s) {
    skiplist_iterator x;

    for (x = skiplist_itr_first(s); x; x =skiplist_itr_next(s, x)) {
        size_t k_sz;
        uint8_t *k;

        k = skiplist_itr_key(s, x, &k_sz);
        fprintf(stderr, "%.*s (%d occurrences)\n", (int)k_sz, k,
                *(int *)skiplist_itr_value(s, x));
    }
}

int run(enum mode mode, char *cclass) {
    int retval = 0;
    FILE *tempfile;

    /* Now read whatever emails we need to, and record the terms which appear
     * in them. */
    token_list = skiplist_new(NULL);

    switch (mode) {
        case error:
            usage(stderr);
            return 1;
            break;

        case train:
            if (!train_read())
                return 1;
            break;

        case test:
        case annotate:
            /* Read a single email. */
            errno = 0;
            if (!read_email(0, stdin, mode == annotate ? &tempfile : 0)) {
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
    
    if (flagD && strchr(flagD, 't')) token_list_dump(token_list);
    
    switch (mode) {
        case train:
            train_update(cclass);
            break;

        case test:
            printf("%s\n", bayes(token_list));
            break;

        case annotate:
            /* Headers of the email have already been written, and remainder
             * saved in tempfile. Compute p(spam), write our header, then dump
             * the rest of the email. */
            printf("X-Bfilter-Class: %s\n", bayes(token_list));
            if (!fdump(tempfile)) retval = 1;
            break;

        case cleandb:
            /* Copy recent data to new database, replace old one. */
            db_clean(28);
            break;

        case stats:
            db_print_stats();
            break;

        case error:
            /* cannot possibly get here, but keep compiler quiet */
            assert(0);
            break;
    }

    db_close();

    skiplist_delete(token_list);

    return retval;
}
