#ifndef SCATTER_GATHER_H
#define SCATTER_GATHER_H

#include <stdint.h>

typedef struct {
    uint64_t address;
    uint64_t length;
} scatter_gather_t;

#endif
