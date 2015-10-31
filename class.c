#include <arpa/inet.h>
#include <assert.h>
#include <string.h>

#include "class.h"
#include "db.h"
#include "line.h"
#include "util.h"

#define CLASSES_KEY "__classes__"

struct class *class_fetch(void) {
    size_t x_sz;
    uint8_t *p, *x;
    struct class *cs = 0;
    int csa = 0, csn = 0;

    x = db_hash_fetch((uint8_t *)CLASSES_KEY, sizeof(CLASSES_KEY) - 1, &x_sz);
    if (x) {
        /* note that the cs array we build here contains pointers into the data
         * x returned from the database; tdb specifies that the caller is
         * responsible for freeing this data, so obviously in this case we
         * don't free */
        for (p = x; p < x + x_sz; ++csn) {
            int i;
            uint32_t nc[3];
            if (csn == csa)
                cs = xrealloc(cs, (csa = csa * 2 + 1) * sizeof *cs);
            cs[csn].name = p;
            while (*p++)
                ;
            for (i = 0; i < 3; ++i) {
                memcpy(nc + i, p, sizeof *nc);
                p += sizeof *nc;
            }
            cs[csn].code = ntohl(nc[0]);
            cs[csn].docs = ntohl(nc[1]);
            cs[csn].terms = ntohl(nc[2]);
        }
    } else
        csa = csn = 0;
        
    /* add two sentinels */
    if (csn == csa)
        cs = xrealloc(cs, (csa += 2) * sizeof *cs);

    cs[csn].name = 0;
    cs[csn].code = cs[csn].docs = cs[csn].terms = 0;

    ++csn;
    cs[csn].name = 0;
    cs[csn].code = cs[csn].docs = cs[csn].terms = 0;

    return cs;
}

_Bool class_store(struct class *cs) {
    struct line csl = { 0 };
    struct class *p;

    for (p = cs; p->code; ++p) {
        int i;
        struct line c = { 0 };
        uint32_t nc[3];

        c.x = p->name;
        c.l = strlen((char *)p->name) + 1; /* including \0 */
        line_cat(&csl, &c);
        nc[0] = htonl(p->code);
        nc[1] = htonl(p->docs);
        nc[2] = htonl(p->terms);
        c.l = 4;
        for (i = 0; i < 3; ++i) {
            c.x = (uint8_t *)(nc + i);
            line_cat(&csl, &c);
        }
    }
    return db_hash_store(
            (uint8_t *)CLASSES_KEY, sizeof(CLASSES_KEY) - 1, csl.x, csl.l);
}

/* warning: overwrites the first sentinel inserted by class_lookup: you
 * can only call class_lookup() once between class_fetch() and
 * class_store() */
struct class *class_lookup(struct class *cs, char *c) {
    int m, n;
    struct class *cp;

    m = 0;
    assert(cs); /* get_classes must abort if it fails to read */
    for (n = 0, cp = cs; cp->code; ++n, ++cp) {
        if (strcmp(c, (char *)cp->name) == 0)
            return cp;
        if (cp->code > m) m = cp->code;
    }

    cp->name = (uint8_t *)c;
    cp->code = m + 1;
    return cp;
}
