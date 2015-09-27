/*
depends test/ubayes
tap
*/

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "bayes.h"
#include "submit.h"

_Bool equal(double a, double b) {
    return fabs(a - b) < 0.0000001;
}

// s0 is 0-terminated, s need not be
_Bool streq(const char *s, const char *s0) {
    return strncmp(s, s0, strlen(s0)) == 0;
}

int db_get_pair(const char *name, int *a, int *b) {
    if (streq(name, "__emails__")) {
        *a = 5; *b = 5;
    }
    if (streq(name, "ham")) {
        *a = 0; *b = 5;
    }
    if (streq(name, "spam")) {
        *a = 5; *b = 0;
    }
    return 1;
}

int main(void) {
    double p;

    printf("1..4\n");

    token_list = skiplist_new(0);
    p = bayes(token_list);
    if (!equal(p, 0.5)) printf("not ");
    printf("ok 1 empty: %f\n", p);

    submit("spam", 4);
    p = bayes(token_list);
    if (!equal(p, 0.99)) printf("not ");
    printf("ok 2 single spam: %f\n", p);

    token_list = skiplist_new(0);
    submit("ham", 3);
    p = bayes(token_list);
    if (!equal(p, 0.01)) printf("not ");
    printf("ok 3 single ham: %f\n", p);

    submit("spam", 4);
    p = bayes(token_list);
    if (!equal(p, 0.5)) printf("not ");
    printf("ok 4 one each: %f\n", p);

    return 0;
}
