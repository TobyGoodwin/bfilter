/*
depends test/ubayes
tap
*/

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "bayes.h"
#include "class.h"
#include "submit.h"

int test = 0;

// s0 is 0-terminated, s need not be
// XXX may read beyond bounds of s
_Bool streq(const uint8_t *s, const char *s0) {
    return strncmp((char *)s, s0, strlen(s0)) == 0;
}

_Bool strneq(const uint8_t *s, const char *s0, size_t n) {
    size_t l = strlen(s0);
    if (n != l) return 0;
    return strncmp((char *)s, s0, l) == 0;
}

#define c_sent { 0 }
#define c_spam { (uint8_t *)"spam", 1, 5, 5 }
#define c_ham { (uint8_t *)"ham", 2, 5, 5 }
struct class empty[] = { c_sent, c_sent };
struct class spam[] = { c_spam, c_sent, c_sent };
struct class both[] = { c_spam, c_ham, c_sent, c_sent };

struct class *class_fetch(void) {
    switch (test) {
        case 1:
            return empty;
        case 2:
            return spam;
        default:
            return both;
    }
}

uint32_t t_spam[] = { 1, 5, 2, 0 };
uint32_t t_ham[] = { 1, 0, 2, 5 };

uint32_t *db_get_intlist(const uint8_t *k, size_t k_sz, unsigned int *n) {
    //fprintf(stderr, "fetch_intlist: %.*s\n", (int)k_sz, k);
    *n = 4;
    if (strneq(k, "spamword", k_sz)) return t_spam;
    if (strneq(k, "hamword", k_sz)) return t_ham;
    *n = 0;
    return 0;
}


uint8_t *db_hash_store(uint8_t *k, size_t k_sz, size_t *d_sz) {
    // should not be called when testing
    assert(0);
}

uint32_t ten = 10;
uint32_t *db_hash_fetch_uint32(uint8_t *k, size_t k_sz) {
    //fprintf(stderr, "fetch_uint32: %.*s\n", (int)k_sz, k);
    if (strneq(k, "--documents--", k_sz)) return &ten;
    if (strneq(k, "--vocabulary--", k_sz)) return &ten;
    return 0;
}

int main(void) {
    struct bayes_result p;

    printf("1..4\n");

    // empty class list => UNKNOWN
    test = 1;
    token_list = skiplist_new(0);
    p = bayes(token_list);
    if (!streq(p.category, "UNKNOWN") ||
            floor(p.range) != 0) printf("not ");
    printf("ok 1 empty: %s (%.1f)\n", p.category, p.range);

    // single class list => that class, but with 0 range
    test = 2;
    submit("spamword", 8);
    submit("spamword", 8);
    p = bayes(token_list);
    if (!streq(p.category, "spam") ||
            floor(p.range) != 0) printf("not ");
    printf("ok 2 single class: %s (%.1f)\n", p.category, p.range);

    // two classes, spam message => spam
    test = 3;
    p = bayes(token_list);
    if (!streq(p.category, "spam")) printf("not ");
    printf("ok 3 spam: %s (%.1f)\n", p.category, p.range);

    // ham message => ham
    token_list = skiplist_new(0);
    submit("hamword", 7);
    submit("hamword", 7);
    p = bayes(token_list);
    if (!streq(p.category, "ham")) printf("not ");
    printf("ok 4 ham: %s (%.1f)\n", p.category, p.range);

    // balanced message => zero range
    submit("spamword", 8);
    submit("spamword", 8);
    p = bayes(token_list);
    if (floor(p.range) != 0) printf("not ");
    printf("ok 5 balanced: %s (%.1f)\n", p.category, p.range);

    return 0;
}
