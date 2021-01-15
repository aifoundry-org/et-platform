#ifndef KERNEL_PARAMS_H
#define KERNEL_PARAMS_H

#include <stdint.h>

typedef struct {
    uint64_t tensor_a;  // Pointer to tensor A
    uint64_t tensor_b;  // Pointer to tensor B
    uint64_t tensor_c;  // Pointer to tensor C
    uint64_t tensor_d;  // Pointer to tensor D
    uint64_t tensor_e;  // Pointer to tensor E
    uint64_t tensor_f;  // Pointer to tensor F
    uint64_t tensor_g;  // Pointer to tensor G
    uint64_t tensor_h;  // Pointer to tensor H
    uint64_t kernel_id; // Id for this Kernel
} kernel_params_t;

#endif
