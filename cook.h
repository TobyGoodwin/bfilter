#include "line.h"

void cook_b64(struct line *);
void cook_entities(struct line *);
void cook_header(struct line *);
void cook_qp(struct line *);
_Bool is_text(struct line *);
