#ifndef BFILTER_H
#define BFILTER_H

#include <stdint.h>

#include "skiplist.h"

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

struct termprob {
    float prob;
    char *term;
    size_t tlen;
};

struct wordcount {
    int nemail, n;
};

uint32_t unbase64(char c);
size_t decode_base64(char *buf, size_t len);
int nemails;
size_t termlength;

int ntokens_submitted;
int history_index, ntokens_history;
struct thist {
    unsigned char term[MAX_TERM_LEN];
    size_t len;
};
struct thist token_history[HISTORY_LEN];

skiplist wordlist;
void record_tokens(void);

void submit_token(char *tok, size_t len);

void submit_text(char *text, size_t len, const int underscores);

int is_b64_chars(const char *buf, size_t len);

size_t nbytesrd;
int read_email(const int fromline, const int passthrough, FILE *fp, FILE **tempfp);

int compare_by_probability(const void *k1, const size_t k1len, const void *k2, const size_t k2len);

#endif
