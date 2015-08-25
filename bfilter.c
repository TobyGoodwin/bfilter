/*
 * bfilter.c:
 * Simple Bayesian email filter, in C.
 *
 * Copyright (c) 2003 Chris Lightfoot. All rights reserved.
 * Email: chris@ex-parrot.com; WWW: http://www.ex-parrot.com/~chris/
 *
 */

static const char rcsid[] = "$Id: bfilter.c,v 1.7 2003/08/19 11:03:21 chris Exp $";

#include <sys/types.h>

#include <errno.h>
#include <gdbm.h>
#include <math.h>
#include <pwd.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <netinet/in.h>     /* for ntohl/htonl */

#include "skiplist.h"
#include "util.h"

/* unbase64 CHAR
 * Decode a single base64 CHAR. */
uint32_t unbase64(char c) {
    static const char *s = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    if (c == '=')
        return 0;
    else
        return (uint32_t)(strchr(s, c) - s);
}

/* decode_base64 BUFFER LENGTH
 * Decode LENGTH characters of base64 data starting at BUFFER in place,
 * returning the number of bytes of real data produced, and padding the
 * end of the original extent with whitespace. */
size_t decode_base64(unsigned char *buf, size_t len) {
    unsigned char *rd, *wr;

    for (rd = buf, wr = buf; rd < buf + len; rd += 4, wr += 3) {
        uint32_t X;
        X = unbase64(rd[3]) | (unbase64(rd[2]) << 6) | (unbase64(rd[1]) << 12) | (unbase64(rd[0]) << 18);
        wr[2] = X & 0xff;
        wr[1] = (X >> 8) & 0xff;
        wr[0] = (X >> 16) & 0xff;
    }

    memset(wr, ' ', len - (wr - buf));

    return wr - buf;
}

/* wordlist is the list of tokens we find; each key is associated with a
 * struct wordcount which stores nemail, the highest-numbered email in which
 * this word was found, and n, the total number of emails in which this word
 * has been found during this session. */
skiplist wordlist;
int nemails;
size_t termlength;
struct wordcount {
    int nemail, n;
};

/* submit_token TOKEN LENGTH
 * Submit an individual LENGTH-character TOKEN to the list of known tokens. */
void submit_token(unsigned char *tok, size_t len) {
    if (len < 2)
        return;
    else if (len > 16 && strncmp(tok, "--", 2) == 0)
        return; /* probably a MIME separator */
    else {
        unsigned char term[32];
        int i, has_alpha = 0;
        struct wordcount *pw;
        
        /* Discard long terms, dates, numbers other than IP numbers. */
        if (len > 32)
            len = 32;

        for (i = 0; i < len; ++i) {
            if (tok[i] > 0xa0 || !strchr("0123456789-@", tok[i]))
                has_alpha = 1;
            if (tok[i] >= 'A' && tok[i] <= 'Z')
                term[i] = (unsigned char)((int)tok[i] + 'a' - 'A');
            else
                term[i] = tok[i];
        }

        if (!has_alpha)
            return;

        pw = skiplist_find(wordlist, term, len);
        if (pw) {
            if (pw->nemail < nemails) {
                pw->nemail = nemails;
                ++pw->n;
            }
        } else {
            struct wordcount w = { 0 };
            w.nemail = nemails;
            w.n = 1;
            skiplist_insert_copy(wordlist, term, len, &w, sizeof w);
            termlength += len;
        }
    }
}

/* submit_text TEXT LENGTH
 * Submit some TEXT for word counting. We discard HTML comments. */
void submit_text(unsigned char *text, size_t len) {
    unsigned char *com_start, *com_end, *p, *tok_start;
    enum tokstate { not_tok = 0, tok, tok_dot } state;

    /* Strip HTML comments. */
    com_start = text;
    while ((com_start = memstr(com_start, len - (com_start - text), "<!--", 4))
            && (com_end = memstr(com_start + 4, len - (com_start + 4 - text), "-->", 3))) {
        memmove(com_start, com_end + 3, len - (com_end + 3 - text));
        len -= com_end + 3 - com_start;
    }

    /* 
     * Now we tokenise the email. Tokens are made up of any of the characters
     * [0-9], [A-Z], [a-z], [\xa0-\xff], and [.@/-], if they have a token
     * character on both sides.
     */
    for (p = text, tok_start = NULL, state = not_tok; p < text + len; ++p) {
        int tok_char, dot;
        tok_char = ((*p >= '0' && *p <= '9')
                    || (*p >= 'A' && *p <= 'Z')
                    || (*p >= 'a' && *p <= 'z')
                    || *p >= 0xa0);
        dot   = (*p == '.' || *p == '-' || *p == '@' || *p == '/');

        switch (state) {
            case not_tok:
                if (tok_char) {
                    tok_start = p;
                    state = tok;
                }
                break;

            case tok:
                if (dot)
                    state = tok_dot;
                else if (!tok_char) {
                    state = not_tok;
                    if (tok_start)
                        submit_token(tok_start, p - tok_start);
                }
                break;

            case tok_dot:
                if (dot || !tok_char) {
                    state = not_tok;
                    if (tok_start)
                        submit_token(tok_start, p - tok_start - 1);
                } else if (tok_char)
                    state = tok;
                break;
        }
    }
    
    /* Submit last token. */
    if (tok_start) {
        if (state == tok)
            submit_token(tok_start, p - tok_start);
        else if (state == tok_dot)
            submit_token(tok_start, p - tok_start - 1);
    }

    /* Done. */
}

/* is_b64_chars BUF LEN
 * Are the first LEN bytes of BUF composed only of base64 characters? */
int is_b64_chars(const unsigned char *buf, size_t len) {
    const unsigned char *p;
    while (len > 0 && buf[len - 1] == '=')
        --len;
    for (p = buf; p < buf + len; ++p)
        if (!((*p >= 'A' && *p <= 'Z')
               || (*p >= 'a' && *p <= 'z')
               || (*p >= '0' && *p <= '9')
               || *p == '+' || *p == '/'))
            return 0;
    return 1;
}

/* read_email FROMLINE PASSTHROUGH STREAM TEMPFILE
 * Read an email from STREAM, and submit the text found using submit_text.
 * Stops at end of file, or, if FROMLINE is nonzero, when a "\n\nFrom " line
 * is found. If PASSTHROUGH is nonzero, we emit the headers to standard output
 * and save the body of the email in a temporary file, which we return as a
 * stdio stream in *TEMPFILE; in this case, any X-Spam-Probability: header is
 * discarded from the email. Returns 1 on success or 0 on failure. */
size_t nbytesrd;
int read_email(const int fromline, const int passthrough, FILE *fp, FILE **tempfp) {
    static size_t buflen;
    static unsigned char *buf;
    int i, j;
    enum parsestate { hdr = 0, hdr_xsp, hdr_rel, bdy_blank, bdy, bdy_b64_1, bdy_b64, end } state = hdr;
    static unsigned char *b64buf = NULL;
    static size_t b64alloc;
    size_t b64len = 0, b64linelen = 0;

    /* 
     * Various tests we use to drive the state machine.
     */
#   define is_blank()      (j == 0)
#   define starts_nwsp()   (j > 0 && !strchr(" \t", buf[0]))
#   define is_hdr_xsp()    ((j >= 19 && strncasecmp(buf, "X-Spam-Probability:", 19) == 0)   \
                            || (j >= 13 && strncasecmp(buf, "X-Spam-Words:", 13) == 0))
#   define is_hdr_rel()    ((j >= 5 && strncasecmp(buf, "From:", 5) == 0)           \
                            || (j >= 8 && strncasecmp(buf, "Subject:", 8) == 0)     \
                            || (j >= 3 && strncasecmp(buf, "To:", 3) == 0)          \
                            || (j >= 14 && strncasecmp(buf, "X-Spam-Status:", 14) == 0))
#   define is_from_()      (fromline && j >= 5 && strncmp(buf, "From ", 5) == 0)
#   define is_b64()        is_b64_chars(buf, j)

    /*
     * Various things to do with identifying and parsing base64 data.
     */

    /* Reset the base64 buffer. */
#   define b64_reset()  \
            do {                    \
                b64linelen = 0;     \
                b64len = 0;         \
            } while (0)
    
    /* Save some candidate base64 data. */
#   define b64_save()    \
            do {                                                                \
                if (!b64buf || b64len + j > b64alloc)                           \
                    b64buf = xrealloc(b64buf, b64alloc = 2 * (b64len + j));     \
                memcpy(b64buf + b64len, buf, j);                                \
                b64len += j;                                                    \
            } while (0)

    /* Decode and submit data from the base64 buffer. */
#   define b64_submit()   \
            do {                                            \
                size_t n, m;                                \
                n = b64len & ~3;                            \
                m = decode_base64(b64buf, n);               \
                submit_text(b64buf, m);                     \
                memmove(b64buf, b64buf + n, b64len - n);    \
                b64len -= n;                                \
            } while (0)

    /* Submit the data from the base64 buffer undecoded, and reset the
     * buffer. */
#   define b64_submit_un()  \
            do {                                \
                submit_text(b64buf, b64len);    \
                b64_reset();                    \
            } while (0)
                            

    if (passthrough && !(*tempfp = tmpfile()))
        return 0;

    if (!buf)
        buf = xmalloc(buflen = 1024);
    
    do {
        /* Obtain a line from the email. */
        j = 0;
        while ((i = getc(fp)) != EOF) {
            buf[j++] = (char)i;
            ++nbytesrd;
            if (j == buflen)
                buf = xrealloc(buf, buflen *= 2);
            if (i == '\n') break;
        }

        --j; /* j is now the number of non-\n characters */
        
        if (ferror(fp))
            goto abort;

        /*
         * State machine. This is quite complicated.
         */
        switch (state) {
            /* hdr: general header */
            case hdr:
                if (is_blank())
                    state = bdy_blank;
                else if (is_hdr_rel())
                    state = hdr_rel;
                else if (is_hdr_xsp())
                    state = hdr_xsp;
                break;

            /* hdr_rel/hdr_xsp: special header */
            case hdr_rel:
            case hdr_xsp:
                if (is_blank())
                    state = bdy_blank;
                else if (is_hdr_rel())
                    state = hdr_rel;
                else if (is_hdr_xsp())
                    state = hdr_xsp;
                else if (starts_nwsp())
                    state = hdr;
                break;

            /* bdy_blank: blank line in body */
            case bdy_blank:
                if (is_from_())
                    state = end;
                else if (j > 32 && is_b64()) {
                    b64_reset();
                    b64_save();
                    b64linelen = j;
                    state = bdy_b64_1;
                } else if (!is_blank())
                    state = bdy;
                break;

            /* bdy: nonblank nonbase64 line in body */
            case bdy:
                if (is_blank())
                    state = bdy_blank;
                break;

            /* bdy_b64_1: candidate first line of base64 data */
            case bdy_b64_1:
                if (is_blank()) {
                    b64_submit_un();
                    state = bdy_blank;
                } else if (j > b64linelen || !is_b64()) {
                    b64_submit_un();
                    state = bdy;
                } else {
                    b64_save();
                    b64_submit();
                    state = bdy_b64;
                }
                break;

            /* bdy_b64: line inside base64 data */
            case bdy_b64:
                if (is_blank())
                    state = bdy_blank;
                else if (j > b64linelen || !is_b64()) {
                    b64_submit_un();
                    state = bdy;
                } else {
                    b64_save();
                    b64_submit();
                }
                break;

            case end:
                /* No way out. */
                break;
        }

        /* End of file ends the email. */
        if (feof(fp)) {
            if (state != bdy && state != bdy_blank)
                goto abort;
            else
                state = end;
        }

        /* Possibly emit/save text. */
        if (passthrough) {
            switch (state) {
                case hdr:
                case hdr_rel:
                    if (fwrite(buf, 1, j + 1, stdout) != j + 1)
                        goto abort;
                    break;

                case hdr_xsp:
                case end:
                    break;

                default:
                    if (fwrite(buf, 1, j + 1, *tempfp) != j + 1)
                        goto abort;
                    break;
            }
        }
        
        /* Pass stuff to the tokeniser. */
        if (state == hdr_rel || state == bdy) {
            unsigned char *p;
            size_t len;
            p = NULL;
            len = (size_t)j;
            if (state == hdr_rel && starts_nwsp() && (p = strchr(buf, ':'))) {
                ++p;
                len -= p - buf;
            } if (!p)
                p = buf;

            submit_text(p, len);
        }
    } while (state != end);
    
    return 1;
    
abort:
    if (passthrough && *tempfp)
        fclose(*tempfp);
    return 0;
    
}

/* usage STREAM
 * Print a usage message to STREAM. */
void usage(FILE *stream) {
    fprintf(stream,
" bfilter isspam      From_ separated emails are read, added to spam corpus\n"
"         isreal                                                real\n"
"         test        An X-Spam-Probability header is added to the email\n"
"                     read from standard input\n"
"         cleandb     Discard little-used terms from the database.\n"
        );
}

static GDBM_FILE filterdb;

void db_abort_func(const char *s) {
    fprintf(stderr, "bfilter: fatal database error: %s\n", s);
    exit(1);
}

/* db_open
 * Open the filter database, which lives in ~/.bfildb. Returns 1 on success
 * or 0 on failure. Because we need to save timestamps, the database is opened
 * read/write in all cases. */
int db_open(void) {
    char *name, *home;
    int i;
    unsigned int delay = 1;

    home = getenv("HOME");
    if (!home) {
        struct passwd *P;
        if (!(P = getpwuid(getuid())))
            return 0;
        home = P->pw_dir;
    }
    name = malloc(strlen(home) + 9);
    sprintf(name, "%s/.bfildb", home);

    /* Wait at most about a minute for the database. */
    for (i = 0; i < 5; ++i) {
        errno = 0;
        filterdb = gdbm_open(name, 0, GDBM_WRCREAT, 0644, db_abort_func);
        if (filterdb || errno != EAGAIN)
            break;
        sleep(delay);
        delay *= 2;
    }
    free(name);

    if (filterdb)
        return 1;
    else
        return 0;
}

/* db_close
 * Close the filter database. */
void db_close(void) {
    gdbm_close(filterdb);
}

/* db_set_pair NAME A B
 * Save under NAME the pair A, B, also setting the timestamp. */
void db_set_pair(const unsigned char *name, unsigned int a, unsigned int b) {
    datum k, d;
    uint32_t u;
    unsigned char data[12];     /* a, b, time of last use. */
    k.dptr = (char*)name;
    k.dsize = strlen(name) + 1;
    u = htonl(a);
    memcpy(data, &u, 4);
    u = htonl(b);
    memcpy(data + 4, &u, 4);
    u = htonl((uint32_t)time(NULL));     /* XXX 2038 bug */
    memcpy(data + 8, &u, 4);
    d.dptr = data;
    d.dsize = 12;
    gdbm_store(filterdb, k, d, GDBM_REPLACE);
}

/* db_get_pair NAME A B
 * Save in *A and *B the elements of the pair identified by NAME, returning 1
 * and updating the timestamp if the pair is found, else returning 0. */
int db_get_pair(const unsigned char *name, unsigned int *a, unsigned int *b) {
    datum k, d;
    k.dptr = (char*)name;
    k.dsize = strlen(name) + 1;
    d = gdbm_fetch(filterdb, k);
    if (!d.dptr || (d.dsize != 8 && d.dsize != 12))
        return 0;
    else {
        uint32_t u;
        
        memcpy(&u, d.dptr, 4);
        *a = ntohl(u);
        memcpy(&u, d.dptr + 4, 4);
        *b = ntohl(u);
        xfree(d.dptr);

        /* Update record with current time. */
        db_set_pair(name, *a, *b);
        
        return 1;
    }
}

/* db_clean DAYS
 * Delete from the database elements which have not been used in the past
 * number of DAYS. */
void db_clean(int ndays) {
    datum k;
    time_t t0;
    unsigned int n = 0, kept = 0, lost = 0;
    
    if (ndays == -1)
        ndays = 28;

    time(&t0);
    t0 -= ndays * 24 * 3600;

    /* Iterate over keys. Note that since we are deleting keys, this might not
     * actually visit all keys. See gdbm(3). */
    for (k = gdbm_firstkey(filterdb); k.dptr; k = gdbm_nextkey(filterdb, k)) {
        if (k.dsize == 8) {
            /* No key; assume that this is older than the threshold. */
            gdbm_delete(filterdb, k);
            ++lost;
        } else {
            uint32_t u;
            time_t t;
            memcpy(&u, k.dptr + 8, 4);
            t = (time_t)ntohl(u);

            if (t < t0) {
                gdbm_delete(filterdb, k);
                ++lost;
            } else
                ++kept;
        }

        xfree(k.dptr);

        if ((++n) % 100 == 0 && isatty(1))
            fprintf(stderr, "Cleaning: kept %8u / discarded %8u\r", kept, lost);
    }
    
    fprintf(stderr, "Cleaning: kept %8u / discarded %8u\n", kept, lost);
    
    /* Actually reclaim disk space. */
    gdbm_reorganize(filterdb);
}

struct termprob {
    float prob;
    unsigned char *term;
    size_t tlen;
};

int compare_by_probability(const void *k1, const size_t k1len, const void *k2, const size_t k2len) {
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

/* main ARGC ARGV
 * Entry point. Usage:
 *
 *  bfilter isspam      From_ separated emails are read, added to spam corpus
 *          isreal                                                real
 *          test        An X-Spam-Probability header is added to the email
 *                      read from standard input
 *          cleandb     Discard terms which have not been used recently.
 */
int main(int argc, char *argv[]) {
    enum { isspam, isreal, test, cleandb } mode;
    skiplist_iterator si;
    FILE *tempfile;
    int retval = 0;

    if (argc != 2) {
        usage(stderr);
        return 1;
    } else if (strcmp(argv[1], "isspam") == 0)
        mode = isspam;
    else if (strcmp(argv[1], "isreal") == 0)
        mode = isreal;
    else if (strcmp(argv[1], "test") == 0)
        mode = test;
    else if (strcmp(argv[1], "cleandb") == 0)
        mode = cleandb;
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
                f = read_email(1, 0, stdin, NULL);
                if (!f) {
                    fprintf(stderr, "bfilter: error while reading email (%s)\n", errno ? strerror(errno) : "no system error");
                    return 1;
                }
             
                /* If we're running on a terminal, print stats. */
                if (isatty(1))
                    fprintf(stderr, "Reading: %8u emails (%8u bytes) %8u terms avg length %8.2f\r", nemails, (unsigned)nbytesrd, skiplist_size(wordlist), (double)termlength / skiplist_size(wordlist));
            } while (!feof(stdin));
            if (isatty(1))
                fprintf(stderr, "\n");
            break;

        case test:
            errno = 0;
            if (!read_email(0, 1, stdin, &tempfile)) {
                fprintf(stderr, "bfilter: failed to read email (%s)\n", errno ? strerror(errno) : "no system error");
                return 1;
            }
            break;

        case cleandb:
            break;
    }

    if (!db_open()) {
        fprintf(stderr, "bfilter: couldn't open database\n");
        return 1;
    }
    
    if (mode == isspam || mode == isreal) {
        /* Update total number of emails and the data for each word. */
        unsigned int nspam, nreal;
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

            if (isatty(1) && (ntermswr % 50) == 0)
                fprintf(stderr, "Writing: %8u / %8u terms (%8u new)\r", ntermswr, nterms, ntermsnew);
        }

        if (isatty(1))
            fprintf(stderr, "Writing: %8u / %8u terms (%8u new)\n", ntermswr, nterms, ntermsnew);

        free(term);
    } else if (mode == test) {
        /* The headers of the email have already been written to standard
         * output; we compute a `spam probability' from the words we've read
         * and those recorded in the database, write out an appropriate
         * header and then dump the rest of the email. */
        int nspamtotal, nrealtotal;
        skiplist problist;
        size_t nterms, n, nsig = 15;
        float a = 1., b = 1., score;
        
        problist = skiplist_new(compare_by_probability);
        db_get_pair("__emails__", &nspamtotal, &nrealtotal);
        
        for (si = skiplist_itr_first(wordlist); si; si = skiplist_itr_next(wordlist, si)) {
            struct termprob t = { 0 };
            int nspam, nreal;
            
            t.term = (unsigned char*)skiplist_itr_key(wordlist, si, &t.tlen);
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
        printf("X-Spam-Words: %d terms\n significant:", nterms);
        if (nsig > nterms)
            nsig = nterms;

        for (si = skiplist_itr_first(problist), n = 0; si && n < nsig; si = skiplist_itr_next(problist, si), ++n) {
            struct termprob *tp;
            tp = (struct termprob*)skiplist_itr_key(problist, si, NULL);
            if (n < 6) {
                /* Avoid emitting high bit characters in the header. */
                unsigned char *p;
                putchar(' ');
                for (p = tp->term; *p; ++p)
                    if (*p < 0x80)
                        putchar(*p);
                    else
                        printf("\\x%02x", (unsigned int)*p);
                printf(" (%6.4f)", tp->prob);
            }
            a *= tp->prob;
            b *= 1. - tp->prob;
        }

        printf("\n");

        if (a == 0.)
            score = 0.;
        else
            score = a / (a + b);
        
        printf("X-Spam-Probability: %s (p=%f)\n", score > 0.9 ? "YES" : "NO", score);

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
    } else if (mode == cleandb) {
        db_clean(28);
    }

    db_close();

    skiplist_delete(wordlist);

    return retval;
}

