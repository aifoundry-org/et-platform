#ifndef KERNEL_PARAMS_H
#define KERNEL_PARAMS_H

#include <stdint.h>

typedef struct {
    uint64_t tensor_a;
    uint64_t tensor_b;
    uint64_t tensor_c;
    uint64_t tensor_d;
    uint64_t tensor_e;
    uint64_t tensor_f;
    uint64_t tensor_g;
    uint64_t tensor_h;
    uint64_t kernel_id;
} kernel_params_t;

#endif
