#ifndef _FPU_TYPES_H
#define _FPU_TYPES_H

#include "softfloat/softfloat.h"

// Floating-point exception flags
enum {
    softfloat_flag_denormal = 32
};

// Arithmetic types
typedef struct { uint16_t v; } float11_t;
typedef struct { uint16_t v; } float10_t;

#endif // _FPU_TYPES_H
