/*
depends test/udb_class
mcheck 'foo,bar,baz,quux' foo bar baz quux
*/

#include <stdint.h>
#include <stdio.h>

#include "class.h"
#include "db.h"
#include "util.h"

int main(int argc, char **argv) {
    int i;
    struct class *cs = xmalloc(10 * sizeof(*cs));

    for (i = 1; i < argc; ++i) {
        cs[i - 1].name = (uint8_t *)argv[i];
        cs[i - 1].code = i;
    }
    cs[i - 1].code = 0;

    db_set_classes(cs);
}
