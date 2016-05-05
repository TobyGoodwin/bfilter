/*
depends test/uclass
export BFILTER_DB=$(mktemp -u)
echo $BFILTER_DB
testdb $BFILTER_DB
mcheck 'foo:1-4-1,bar:2-5-4,baz:3-6-9,quux:4-7-16,' foo bar baz quux
mcheck 'foo:1-4-1,bar:2-5-4,baz:3-6-9,quux:4-7-16,'
mcheck 'foo:1-4-1,bar:2-5-4,baz:3-6-9,quux:4-7-16,red:5-4-1,:6-5-4,' red ''
*/

#include <stdint.h>
#include <stdio.h>

#include "db.h"
#include "class.h"
#include "util.h"

int main(int argc, char **argv) {
    char *errmsg, q[100];
    int i, n;
    struct class *c;

    db_write();
    for (i = 1; i < argc; ++i) {
        snprintf(q, 100, "\
insert into class (name, docs, terms) values ('%s', %d, %d); \
", argv[i], i + 3, i * i);
        sqlite3_exec(db_db(), q, 0, 0, &errmsg);
        if (errmsg) fprintf(stderr, "%s\n", errmsg);
    }

    c = class_fetch(&n, 0);
    for (i = 0; i < n; ++i, ++c)
        printf("%s:%d-%d-%d,", c->name, c->id, c->docs, c->terms);
    printf("\n");
}
