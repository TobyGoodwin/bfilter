#include <stdint.h>

#include "count.h"
#include "db.h"

/* add n to the count for token t of size t_sz and class (code) c */
void count_add(uint8_t *t, size_t t_sz, int c, int n) {
    _Bool gotit = 0;
    int i, n_is;
    uint32_t *is, *x;

    is = db_get_intlist(t, t_sz, &n_is);
    if (!is || n_is % 2 != 0) {
        x = xmalloc(2 * sizeof *x);
        x[0] = c;
        x[1] = n;
        db_set_intlist(t, t_sz, x, 2);
        return;
    }
    for (i = 0; !gotit && i < n_is; i += 2) {
        if (is[i] == c) {
            gotit = 1;
            is[i + 1] += n;
        }
    }
    if (gotit)
        x = is;
    else {
        int new = n_is + 2;
        x = xmalloc(new * sizeof *x);
        memcpy(x, is, n_is * sizeof *x);
        x[n_is] = c;
        x[n_is + 1] = n;
        n_is = new;
    }
    db_set_intlist(t, t_sz, x, n_is);
}
