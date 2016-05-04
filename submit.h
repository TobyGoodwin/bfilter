#include "skiplist.h"

/* XXX these should not be global variables */
skiplist token_list;
int nemails, ntokens_submitted;
size_t term_length;

void submit(char *, size_t);
