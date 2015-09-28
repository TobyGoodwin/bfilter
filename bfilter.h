#include <stdint.h>

#include "skiplist.h"

enum mode { isspam, isreal, test, annotate, cleandb, stats } mode;

/* Global variables are bad, m'kay? */
_Bool flagb;
int nemails;
size_t nbytesrd, termlength;
skiplist wordlist;
