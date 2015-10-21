/*
depends test/udb_class
mcheck 'foo:1,bar:2,baz:3,quux:4,' foo bar baz quux
mcheck ''
mcheck 'foo:1,:2,baz:3,quux:4,' foo '' baz quux
*/

#include <stdint.h>
#include <stdio.h>

#include "class.h"
#include "db.h"
#include "util.h"

int main(int argc, char **argv) {
    int i;
    struct class *cs = xmalloc(10 * sizeof(*cs));
    struct class *r;

    for (i = 1; i < argc; ++i) {
        cs[i - 1].name = (uint8_t *)argv[i];
        cs[i - 1].code = i;
    }
    cs[i - 1].code = 0;

    if (!db_open()) return 1;
    db_set_classes(cs);

    for (r = db_get_classes(); r->code; ++r)
        printf("%s:%d,", r->name, r->code);
    printf("\n");
}
