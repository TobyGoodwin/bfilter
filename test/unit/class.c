/*
depends test/uclass
export BFILTER_DB=$(mktemp -u)
echo $BFILTER_DB
mcheck 'foo:1-4-1,bar:2-5-4,baz:3-6-9,quux:4-7-16,' foo bar baz quux
mcheck 'foo:1-4-1,bar:2-5-4,baz:3-6-9,quux:4-7-16,'
mcheck 'foo:3-6-9,bar:2-5-4,baz:3-6-9,quux:4-7-16,red:1-4-1,:2-5-4,' red '' foo
*/

#include <stdint.h>
#include <stdio.h>

#include "db.h"
#include "class.h"
#include "util.h"

int main(int argc, char **argv) {
    int i;
    struct class *c, *cs;

    if (!db_open()) return 1;
    for (i = 1; i < argc; ++i) {
        cs = class_fetch();
        c = class_lookup(cs, argv[i]);
        c->code = i;
        c->docs = i + 3;
        c->terms = i * i;
        class_store(cs);
    }

    for (c = class_fetch(); c->code; ++c)
        printf("%s:%d-%d-%d,", c->name, c->code, c->docs, c->terms);
    printf("\n");
}
