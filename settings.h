/* HISTORY_LEN
 * The number of terms we may amalgamate into a single token. You can tweak
 * this; larger numbers use more database space, but should give more accurate
 * discrimination of spam and nonspam. */
#define HISTORY_LEN     3

/* MAX_TOKENS
 * Largest number of tokens we generate from a single mail. */
#define MAX_TOKENS      300

/* MAX_TERM_LEN
 * Largest term we consider. */
#define MAX_TERM_LEN    32

