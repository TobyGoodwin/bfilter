#define BFILTER_VERSION "1.0"

/* HISTORY_LEN
 * The number of tokens we may amalgamate into a single term. You can
 * tweak this; larger numbers use more database space, but should give
 * more accurate discrimination of spam and nonspam. */
#define HISTORY_LEN     2

/* MAX_TERM_LEN
 * Largest term we consider. */
#define MAX_TERM_LEN    32

/* DATABASE_NAME
 * default database name (relative to $HOME) */
#define DATABASE_NAME ".bfilter.sqlite"
