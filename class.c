#include <string.h>

#include "class.h"
#include "db.h"
#include "util.h"

int class_lookup(char *c) {
    int m, n;
    struct class *cp, *cs = db_get_classes();

    m = 0;
    if (!cs) n = 2;
    else for (n = 0, cp = cs; cp->code; ++n, ++cp) {
        if (strcmp(c, (char *)cp->name) == 0)
            return cp->code;
        if (cp->code > m) m = cp->code;
    }

    cp = xmalloc(n * sizeof *cp);
    if (cs) memcpy(cp, cs, (n - 1) * sizeof *cp);
    cp[n - 2].name = (uint8_t *)c;
    cp[n - 2].code = m + 1;
    cp[n - 1].name = 0;
    cp[n - 1].code = 0; /* sentinel */

    db_set_classes(cp);
    return m + 1;
}
