#ifndef BFILTER_H
#define BFILTER_H

#include <stdint.h>

#include "skiplist.h"

#define SS(x)   #x

struct termprob {
    float prob;
    char *term;
    size_t tlen;
};

uint32_t unbase64(char c);
size_t decode_base64(char *buf, size_t len);
int nemails;
size_t termlength;

skiplist wordlist;
void record_tokens(void);

void submit_token(char *tok, size_t len);

void submit_text(char *text, size_t len, const int underscores);

int is_b64_chars(const char *buf, size_t len);

size_t nbytesrd;
int read_email(const int fromline, const int passthrough, FILE *fp, FILE **tempfp);

int compare_by_probability(const void *k1, const size_t k1len, const void *k2, const size_t k2len);

#endif
