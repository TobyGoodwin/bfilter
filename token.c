// for fprinf only
#include <stdio.h>

#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "compose.h"
#include "settings.h"
#include "submit.h"
#include "token.h"
#include "util.h"

void token_submit(uint8_t *t, size_t l) {
    int i, has_alpha = 0;

    /* XXX probably want to move this test higher, as there's no point
     * continuing to tokenize if we've reached the limit. */
#if 0
    if (ntokens_submitted > max_tokens) {
        return;
    }
#endif
    if (l < 2)
        return;
    if (l > 16 && strncmp((const char *)t, "--", 2) == 0)
        return; /* probably a MIME separator */

    /* Truncate long terms */
    if (l > MAX_TERM_LEN)
        return;
        //l = MAX_TERM_LEN;

    /* Fold to lower case, check for letters. */
    for (i = 0; i < l; ++i) {
        if (t[i] > 0xa0 || !strchr("0123456789-_.@/", t[i]))
            has_alpha = 1;
    }

    /* Discard dates, numbers, etc. */
    if (!has_alpha) {
        //fprintf(stderr, "discarding %.*s\n", (int)l, term);
        return;
    }

    compose(t, l);
}

/* submit_text TEXT LENGTH UNDERSCORES
 * Submit some TEXT for word counting. We discard HTML comments. If UNDERSCORES
 * is true, then an underscore does NOT terminate a token; this helps us deal
 * with the tokens generated by SpamAssassin. */
void tokenize(uint8_t *text, size_t len, const _Bool header) {
    uint8_t *com_start, *com_end, *p, *tok_start;
    enum tokstate { not_tok = 0, tok, tok_dot, bra_ket } state;

    com_start = text;
    while ((com_start = (uint8_t *)memstr(com_start,
                    len - (com_start - text), (uint8_t *)"<!--", 4))
            && (com_end = (uint8_t *)memstr(com_start + 4,
                    len - (com_start + 4 - text), (uint8_t *)"-->", 3))) {
        memmove(com_start, com_end + 3, len - (com_end + 3 - text));
        len -= com_end + 3 - com_start;
    }

    /* 
     * Now we tokenise the email. Tokens are made up of any of the characters
     * [0-9], [A-Z], [a-z], [\x80-\xff], and ['.@/-] if they have a token
     * character on both sides.
     */
    for (p = text, tok_start = NULL, state = not_tok; p < text + len; ++p) {
        _Bool bra, ket, tok_char, dot;

        tok_char = ((*p >= '0' && *p <= '9')
                    || (*p >= 'A' && *p <= 'Z')
                    || (*p >= 'a' && *p <= 'z')
		    || *p == '-'
                    || *p >= 0x80);

        bra = !header && *p == '<';
        ket = !header && *p == '>';
        
        dot = (*p == '.'
               || *p == '@'
               || *p == '\''
               || *p == '/'
               || *p == '+'
               || (header && *p == '_'));

        switch (state) {
            case not_tok:
                if (tok_char) {
                    tok_start = p;
                    state = tok;
                }
                if (bra)
                    state = bra_ket;
                break;

            case tok:
                if (dot)
                    state = tok_dot;
                else if (!tok_char) {
                    state = bra ? bra_ket : not_tok;
                    if (tok_start)
                        token_submit(tok_start, p - tok_start);
                }
                break;

            case tok_dot:
                if (dot || !tok_char) {
                    state = bra ? bra_ket : not_tok;
                    if (tok_start)
                        token_submit(tok_start, p - tok_start - 1);
                } else if (tok_char)
                    state = tok;
                break;

            case bra_ket:
                if (ket)
                    state = not_tok;
                break;
        }
    }
    
    /* Submit last token. */
    if (tok_start) {
        if (state == tok)
            token_submit(tok_start, p - tok_start);
        else if (state == tok_dot)
            token_submit(tok_start, p - tok_start - 1);
    }

    /* Done. */
}
