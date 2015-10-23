#include <assert.h>
#include <string.h>

#include "class.h"
#include "db.h"
#include "util.h"

int class_lookup(char *c) {
    int m, n;
    struct class *cp, *cs = db_get_classes();

    m = 0;
    assert(cs); /* get_classes must abort if it fails to read */
    for (n = 0, cp = cs; cp->code; ++n, ++cp) {
        if (strcmp(c, (char *)cp->name) == 0)
            return cp->code;
        if (cp->code > m) m = cp->code;
    }

    cp = xmalloc((n + 2) * sizeof *cp);
    memcpy(cp, cs, n * sizeof *cp);
    ++m;
    cp[n].name = (uint8_t *)c;
    cp[n].code = m;
    ++n;
    cp[n].name = 0;
    cp[n].code = 0; /* sentinel */

    db_set_classes(cp);
    return m;
}
