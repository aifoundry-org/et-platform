#ifndef KERNEL_INFO_H
#define KERNEL_INFO_H

#include "kernel_params.h"

#include <stdint.h>

typedef struct {
    uint64_t compute_pc;
    uint64_t uber_kernel_nodes; // TODO FIXME remove when no longer needed
    uint64_t shire_mask;
    kernel_params_t *kernel_params_ptr;
} kernel_info_t;

#endif
