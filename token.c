// for fprinf only
#include <stdio.h>

#include <string.h>
#include <unistd.h>

#include "settings.h"
#include "compose.h"
#include "util.h"

void token_submit(char *t, size_t l) {
    char term[MAX_TERM_LEN];
    int i, has_alpha = 0;

    if (l < 2 || ntokens_submitted > MAX_TOKENS)
        return;
    else if (l > 16 && strncmp(t, "--", 2) == 0)
        return; /* probably a MIME separator */

    /* Truncate long terms */
    if (l > MAX_TERM_LEN)
        l = MAX_TERM_LEN;

    /* Fold to lower case, check for letters. */
    for (i = 0; i < l; ++i) {
        if (t[i] > 0xa0 || !strchr("0123456789-_.@/", t[i]))
            has_alpha = 1;
        if (t[i] >= 'A' && t[i] <= 'Z')
            term[i] = t[i] + 'a' - 'A';
        else
            term[i] = t[i];
    }

    /* Discard dates, numbers, etc. */
    if (!has_alpha) {
        fprintf(stderr, "discarding %.*s\n", (int)l, term);
        return;
    }

    submit(term, l);
}

/* submit_text TEXT LENGTH UNDERSCORES
 * Submit some TEXT for word counting. We discard HTML comments. If UNDERSCORES
 * is true, then an underscore does NOT terminate a token; this helps us deal
 * with the tokens generated by SpamAssassin. */
void tokenize(char *text, size_t len, const int underscores) {
    char *com_start, *com_end, *p, *tok_start;
    enum tokstate { not_tok = 0, tok, tok_dot } state;

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
                        token_submit(tok_start, p - tok_start);
                }
                break;

            case tok_dot:
                if (dot || !tok_char) {
                    state = not_tok;
                    if (tok_start)
                        token_submit(tok_start, p - tok_start - 1);
                } else if (tok_char)
                    state = tok;
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
