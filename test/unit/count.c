/*
true
*/

/*
depends test/ucount
export BFILTER_DB=$(mktemp -u)
mcheck '3,1,' 3
mcheck '3,2,' 3
mcheck '3,2,5,2,' 5 5
*/

// XXX we could definitely do with some tests of db_count_update

#include <stdio.h>
#include <stdlib.h>

#include "count.h"
#include "db.h"

int main(int argc, char **argv) {
    int i, n;
    uint32_t *r;

    if (!db_open()) return 1;
    for (i = 1; i < argc; ++i) {
        int c = atoi(argv[i]);
        count_update(1, 3, c, 1);
    }
    r = db_get_intlist((uint8_t *)"bar", 3, &n);
    for (i = 0; i < n; ++i)
        printf("%d,", r[i]);
    printf("\n");

}
