#include <unistd.h>

typedef void (*token_processor)(char *, size_t);

void tokenize(char *, size_t, const int, token_processor proc);

