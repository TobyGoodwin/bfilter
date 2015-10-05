#include <stdio.h>

#include "bfilter.h"
#include "read.h"
#include "util.h"

/* the current line, as read */
char *line = 0;
size_t l = 0, line_alloc = 0;

void read_line(FILE *in) {
    int i;

    l = 0;
    while ((i = getc(in)) != EOF) {
        if (l == line_alloc)
            line = xrealloc(line, line_alloc += line_alloc + 1);
        line[l++] = (char)i;
        ++nbytesrd;
        if (i == '\n') break;
    }
}

_Bool write_line(FILE *out) {
    return fwrite(line, 1, l, out) == l;
}

int read_email(const _Bool fromline, FILE *in, FILE **tmp) {
    /* If tmp is set, we are in "passthrough" mode; *tmp is the FILE * where we
     * will stash the body of the email, to be output later. */
    if (tmp && !(*tmp = tmpfile()))
            return 0;

    while (1) {
        read_line(in);
        if (l == 0) break;
        if (tmp)
            if (!write_line(stdout))
                goto abort;
    }
    return 1;

abort:
    if (*tmp)
        fclose(*tmp);
    return 0;
}
