#include <stdint.h>

#include "skiplist.h"

enum mode { error, train, test, annotate, cleandb, stats } mode;

/* Global variables are bad, m'kay? */
_Bool flagb;
char *flagD;

int nemails;
size_t nbytesrd, termlength;
skiplist wordlist;
