#include <stdint.h>
#include <unistd.h>

#define EMAILS_KEY "__emails__"
#define EMAILS_CLASS_KEY "__emailsperclass__"
#define TERMS_CLASS_KEY "__termsperclass__"
#define VOCAB_KEY "__vocabulary__"

_Bool count_add(uint8_t *t, size_t t_sz, int c, int n);
