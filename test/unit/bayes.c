/*
depends test/ubayes
export BFILTER_DB=$(mktemp -u)
echo $BFILTER_DB
testdb $BFILTER_DB
tap
*/

#include <assert.h>
#include <math.h>
#include <sqlite3.h>
#include <stdio.h>
#include <string.h>

#include "bayes.h"
#include "class.h"
#include "db.h"
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

#if 0
#define c_spam { 1, (uint8_t *)"spam", 5, 5, 0.0 }
#define c_ham { 2, (uint8_t *)"ham", 5, 5, 0.0 }
struct class empty[] = { };
struct class spam[] = { c_spam };
struct class both[] = { c_spam, c_ham };

_Bool db_term_id_fetch(uint8_t *t, int tl, int *x) {
    if (strneq(t, "spamword", tl)) *x = 4;
    if (strneq(t, "hamword", tl)) *x = 3;
    return 1;
}

void db_stmt_ready(void) {
}

void db_fatal(char *p, char *q) {
}

struct db_stmt { int x; };
void db_stmt_finalize(struct db_stmt *s) {
}

struct class *class_fetch(int *x, int **y) {
    switch (test) {
        case 1:
            *x = 0;
            return empty;
        case 2:
            *x = 1;
            return spam;
        default:
            *x = 2;
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

int db_documents(void) {
    return 5;
}

int db_vocabulary(void) {
    return 10;
}
#endif

int main(void) {
    char *errmsg;
    double gap;
    int p_n;
    struct class *p;

    db_open();
    printf("1..5\n");

    // empty class list => empty result
    test = 1;
    token_list = skiplist_new(0);
    p = bayes(token_list, &p_n);
    if (p_n != 0) printf("not ");
    printf("ok 1 empty\n");

    // single class list => that class
    test = 2;
    sqlite3_exec(db_db(), "\
insert into class (name, docs, terms) values ('spam', 1, 1); \
", 0, 0, &errmsg);
    if (errmsg) fprintf(stderr, "%s\n", errmsg);
    p = bayes(token_list, &p_n);
    if (!streq(p[0].name, "spam") || p_n != 1) printf("not ");
    printf("ok 2 single class: %s\n", p[0].name);

    // two classes, spam message => spam
    test = 3;
    sqlite3_exec(db_db(), "\
insert into class (name, docs, terms) values ('ham', 1, 1); \
insert into count (class, term, count) values (1, 1, 3); \
insert into count (class, term, count) values (2, 2, 3); \
", 0, 0, &errmsg);
    if (errmsg) fprintf(stderr, "%s\n", errmsg);
    submit("spamword", 8);
    submit("spamword", 8);
    p = bayes(token_list, &p_n);
    gap = p[0].logprob - p[1].logprob;
    if (!streq(p[0].name, "spam") || p_n != 2) printf("not ");
    printf("ok 3 spam: %s (%.1f)\n", p[0].name, gap);

    // ham message => ham
    token_list = skiplist_new(0);
    submit("hamword", 7);
    submit("hamword", 7);
    p = bayes(token_list, &p_n);
    gap = p[0].logprob - p[1].logprob;
    if (!streq(p[0].name, "ham")) printf("not ");
    printf("ok 4 ham: %s (%.1f)\n", p[0].name, gap);

    // balanced message => zero range
    submit("spamword", 8);
    submit("spamword", 8);
    p = bayes(token_list, &p_n);
    gap = p[0].logprob - p[1].logprob;
    if (gap > 0.1) printf("not ");
    printf("ok 5 balanced: %s (%.1f)\n", p[0].name, gap);

    return 0;
}
