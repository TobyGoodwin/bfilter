#include "skiplist.h"

/* token_list is the list of tokens we find; each key is associated with a
 * struct wordcount which stores nemail, the highest-numbered email in which
 * this word was found, and n, the total number of emails in which this word
 * has been found during this session. */
/* XXX these should not be global variables */
skiplist token_list;
int nemails, ntokens_submitted;
size_t term_length;

void submit(char *, size_t);
