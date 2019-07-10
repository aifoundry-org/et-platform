#ifndef KERNEL_INFO_H
#define KERNEL_INFO_H

#include "kernel_params.h"

#include <stdint.h>
typedef uint64_t grid_config_t; // TODO

typedef struct {
    uint64_t shire_mask;
    uint64_t compute_pc;
    kernel_params_t* kernel_params_ptr;
    grid_config_t* grid_config_ptr;
} kernel_info_t;

#endif
