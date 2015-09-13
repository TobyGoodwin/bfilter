/*
 * bfilter.c:
 * Simple Bayesian email filter, in C.
 *
 * Copyright (c) 2003 Chris Lightfoot. All rights reserved.
 * Email: chris@ex-parrot.com; WWW: http://www.ex-parrot.com/~chris/
 *
 */

static const char rcsid[] = "$Id: bfilter.c,v 1.24 2005/06/07 16:41:22 chris Exp $";

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

#include "bfilter.h"
#include "db.h"
#include "skiplist.h"
#include "util.h"

/* HISTORY_LEN
 * The number of terms we may amalgamate into a single token. You can tweak
 * this; larger numbers use more database space, but should give more accurate
 * discrimination of spam and nonspam. */
#define HISTORY_LEN     3

/* MAX_TOKENS
 * Largest number of tokens we generate from a single mail. */
#define MAX_TOKENS      3000

/* MAX_TERM_LEN
 * Largest term we consider. */
#define MAX_TERM_LEN    32
#define SS(x)   #x

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
size_t decode_base64(char *buf, size_t len) {
    char *rd, *wr;

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

static int ntokens_submitted;
static int history_index, ntokens_history;
struct {
    unsigned char term[MAX_TERM_LEN];
    size_t len;
} token_history[HISTORY_LEN];

/* wordlist is the list of tokens we find; each key is associated with a
 * struct wordcount which stores nemail, the highest-numbered email in which
 * this word was found, and n, the total number of emails in which this word
 * has been found during this session. */
skiplist wordlist;
int nemails;
size_t termlength;

/* record_tokens
 * Record the most recently submitted token, and composite tokens from the
 * history. */
void record_tokens(void) {
    unsigned char term[(MAX_TERM_LEN + 1) * HISTORY_LEN];
    struct wordcount *pw;
    int n;

    for (n = 1; n <= ntokens_history; ++n) {
        unsigned char *p;
        int i;
        for (i = 0, p = term; i < n; ++i) {
            int j;
            if (i > 0) *(p++) = '%';
            j = (history_index - n + i + HISTORY_LEN) % HISTORY_LEN;
            memcpy(p, token_history[j].term, token_history[j].len);
            p += token_history[j].len;
        }

        pw = skiplist_find(wordlist, term, p - term);
        if (pw) {
            if (pw->nemail < nemails) {
                pw->nemail = nemails;
                ++pw->n;
            }
        } else {
            struct wordcount w = { 0 };
            w.nemail = nemails;
            w.n = 1;
            skiplist_insert_copy(wordlist, term, p - term, &w, sizeof w);
            termlength += p - term;
            ++ntokens_submitted;
        }
    }
}

/* submit_token TOKEN LENGTH
 * Submit an individual LENGTH-character TOKEN to the list of known tokens. */
void submit_token(char *tok, size_t len) {
    if (len < 2 || ntokens_submitted > MAX_TOKENS)
        return;
    else if (len > 16 && strncmp(tok, "--", 2) == 0)
        return; /* probably a MIME separator */
    else {
        unsigned char term[MAX_TERM_LEN];
        int i, has_alpha = 0;
        
        /* Discard long terms, dates, numbers other than IP numbers. */
        if (len > MAX_TERM_LEN)
            len = MAX_TERM_LEN;

	fprintf(stderr, "%.*s ", (int)len, tok);
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

        /* Update history. */
        memcpy(token_history[history_index].term, term, len);
        token_history[history_index].len = len;
        history_index = (history_index + 1) % HISTORY_LEN;
        if (ntokens_history < HISTORY_LEN)
            ++ntokens_history;

        /* Submit this token and composites with preceding ones. */
        record_tokens();
    }
}


/* submit_text TEXT LENGTH UNDERSCORES
 * Submit some TEXT for word counting. We discard HTML comments. If UNDERSCORES
 * is true, then an underscore does NOT terminate a token; this helps us deal
 * with the tokens generated by SpamAssassin. */
void submit_text(char *text, size_t len, const int underscores) {
    char *com_start, *com_end, *p, *tok_start;
    enum tokstate { not_tok = 0, tok, tok_dot } state;

//fprintf(stderr, "submit_text: %.*s\n", len, text);
    /* Strip HTML comments. */
    com_start = text;
    while ((com_start = (char *)memstr(com_start, len - (com_start - text), "<!--", 4))
            && (com_end = (char *)memstr(com_start + 4, len - (com_start + 4 - text), "-->", 3))) {
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
		    || *p == '-'
                    || *p >= 0xa0);
        
        dot = (*p == '.'
               || *p == '@'
               || *p == '/'
               || (underscores && *p == '_'));

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
int is_b64_chars(const char *buf, size_t len) {
    const char *p;
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
    static char *buf;
    int i, j;
    enum parsestate { hdr = 0, hdr_xsp, hdr_rel, bdy_blank, bdy, bdy_b64_1, bdy_b64, end } state = hdr;
    static char *b64buf = NULL;
    static size_t b64alloc;
    size_t b64len = 0, b64linelen = 0;

    ntokens_submitted = 0;  /* ugh. */
    ntokens_history = 0;    /* ugh again. */

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
                submit_text(b64buf, m, 0);                  \
                memmove(b64buf, b64buf + n, b64len - n);    \
                b64len -= n;                                \
            } while (0)

    /* Submit the data from the base64 buffer undecoded, and reset the
     * buffer. */
#   define b64_submit_un()  \
            do {                                \
                submit_text(b64buf, b64len, 0); \
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
         * State machine. This is quite complicated. See tokeniser-states.dot
         * for a pretty (albeit not-very-helpful) diagram.
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
            char *p;
            size_t len;
            p = NULL;
            len = (size_t)j;
            if (state == hdr_rel && starts_nwsp() && (p = strchr(buf, ':'))) {
                ++p;
                len -= p - buf;
            } if (!p)
                p = buf;

            submit_text(p, len, state == hdr_rel);
        }
    } while (state != end);
    
    return 1;
    
abort:
    if (passthrough && *tempfp)
        fclose(*tempfp);
    return 0;
    
}

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
