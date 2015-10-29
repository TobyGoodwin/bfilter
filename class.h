#ifndef CLASS_H
#define CLASS_H

#include <stdint.h>

struct class {
    uint8_t *name; /* nul terminated */
    uint32_t code; /* unique id for "foreign key" */
    uint32_t docs; /* number of documents in this class */
    uint32_t terms; /* total number of terms (inc dups) in docs in class */
};

struct class *class_fetch(void);
int class_lookup(char *);
_Bool class_store(struct class *);

#endif
