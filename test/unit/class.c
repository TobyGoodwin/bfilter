/*
depends test/uclass
export BFILTER_DB=$(mktemp)
echo $BFILTER_DB
mcheck 'foo:1-4-1,bar:2-5-4,baz:3-6-9,quux:4-7-16,' foo bar baz quux
mcheck ''
mcheck 'foo:1-4-1,:2-5-4,baz:3-6-9,quux:4-7-16,' foo '' baz quux
*/

#include <stdint.h>
#include <stdio.h>

#include "db.h"
#include "class.h"
#include "util.h"

int main(int argc, char **argv) {
    int i;
    struct class *cs = xmalloc(10 * sizeof(*cs));
    struct class *r;

    for (i = 1; i < argc; ++i) {
        cs[i - 1].name = (uint8_t *)argv[i];
        cs[i - 1].code = i;
        cs[i - 1].docs = i + 3;
        cs[i - 1].terms = i * i;
    }
    cs[i - 1].code = 0;

    if (!db_open()) return 1;
    class_store(cs);

    for (r = class_fetch(); r->code; ++r)
        printf("%s:%d-%d-%d,", r->name, r->code, r->docs, r->terms);
    printf("\n");
}
