/* HISTORY_LEN
 * The number of tokens we may amalgamate into a single term. You can
 * tweak this; larger numbers use more database space, but should give
 * more accurate discrimination of spam and nonspam. */
#define HISTORY_LEN     4

/* MAX_TOKENS
 * Largest number of tokens we generate from a single mail, when
 * training... */
#define MAX_TRAIN_TOKENS 5000
/* ...and when testing. */
#define MAX_TEST_TOKENS 500
unsigned int max_tokens;

/* MAX_TERM_LEN
 * Largest term we consider. */
#define MAX_TERM_LEN    32

/* UNSURE_LOG_RANGE
 * if the range of log probabilities is smaller than this double, classify as
 * UNSURE */
#define UNSURE_LOG_RANGE 3.
