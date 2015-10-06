/*
depends test/uline
tap
*/

#include <stdio.h>
#include <string.h>

#include "line.h"

int main(void) {
    struct line l1 = { 0 }, l2 = { 0 };

    printf("1..2\n");
    l2.x = "hello";
    l2.l = strlen(l2.x);

    line_cat(&l1, &l2);
    if (strncmp(l1.x, "hello", 5) != 0)
        printf("not ");
    printf("ok 1 cat to empty\n");

    l2.x = "world";
    line_cat(&l1, &l2);
    if (strncmp(l1.x, "helloworld", 10) != 0)
        printf("not ");
    printf("ok 2 cat to non-empty\n");

    return 0;
}
