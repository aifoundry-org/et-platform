#ifndef KERNEL_CONFIG_H
#define KERNEL_CONFIG_H

#include "kernel_info.h"
#include "kernel_params.h"

#include <stdint.h>

#define KERNEL_LAUNCH_FLAGS_EVICT_L3_BEFORE_LAUNCH (1 << 0)

typedef struct
{
    kernel_info_t kernel_info;
    kernel_params_t kernel_params;
    uint64_t num_shires;
    uint64_t kernel_launch_flags;
} kernel_config_t;

#endif
