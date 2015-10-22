/*
depends test/udb_intlist
export BFILTER_DB=$(mktemp)
mcheck '1,1,2,4,3,9,4,16,' a b c d
*/

#include <stdint.h>
#include <stdio.h>

#include "class.h"
#include "db.h"
#include "util.h"

int main(int argc, char **argv) {
    int i, n;
    uint32_t *r, *x = xmalloc(10 * 4);

    for (i = 1; i < argc; ++i) {
        x[(i - 1) * 2] = i;
        x[(i - 1) * 2 + 1] = i*i;
    }

    if (!db_open()) return 1;
    db_set_intlist((uint8_t *)"foo", 3, x, (argc - 1) * 2);

    r = db_get_intlist((uint8_t *)"foo", 3, &n);
    for (i = 0; i < n; ++i)
        printf("%d,", r[i]);
    printf("\n");
}
