/* HISTORY_LEN
 * The number of tokens we may amalgamate into a single term. You can
 * tweak this; larger numbers use more database space, but should give
 * more accurate discrimination of spam and nonspam. */
#define HISTORY_LEN     3

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

/* SIGNIFICANT_TERMS
 * Once we have collected terms, we order them by significance. Only the
 * first few most significant terms are used to calculate the
 * probability for a message. */
#define SIGNIFICANT_TERMS 23

/* SPAM_THRESHOLD
 * If p > SPAM_THRESHOLD, label this message as spam. */
#define SPAM_THRESHOLD 0.9
