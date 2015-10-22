#ifndef CLASS_H
#define CLASS_H

#include <stdint.h>

struct class {
    uint32_t code;
    uint8_t *name;
};

int class_lookup(char *);

#endif
