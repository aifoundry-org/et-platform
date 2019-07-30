#ifndef KERNEL_CONFIG_H
#define KERNEL_CONFIG_H

#include "kernel_info.h"
#include "kernel_params.h"

#include <stdint.h>

typedef struct
{
    kernel_info_t kernel_info;
    kernel_params_t kernel_params;
    uint64_t num_shires;
} kernel_config_t;

#endif
