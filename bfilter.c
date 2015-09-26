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
#include "compose.h"
#include "db.h"
#include "skiplist.h"
#include "token.h"
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
 * Read an email from STREAM, tokenize and submit.
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

    compose_reset();

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
                tokenize(b64buf, m, 0);       \
                memmove(b64buf, b64buf + n, b64len - n);    \
                b64len -= n;                                \
            } while (0)

    /* Submit the data from the base64 buffer undecoded, and reset the
     * buffer. */
#   define b64_submit_un()                                 \
            do {                                           \
                tokenize(b64buf, b64len, 0); \
                b64_reset();                               \
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

            tokenize(p, len, state == hdr_rel);
        }
    } while (state != end);
    
    return 1;
    
abort:
    if (passthrough && *tempfp)
        fclose(*tempfp);
    return 0;
    
}

