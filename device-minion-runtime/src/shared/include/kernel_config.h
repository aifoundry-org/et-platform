#ifndef KERNEL_CONFIG_H
#define KERNEL_CONFIG_H

#include <stdint.h>

#define KERNEL_LAUNCH_FLAGS_EVICT_L3_BEFORE_LAUNCH (1u << 0)
#define KERNEL_LAUNCH_FLAGS_EVICT_L3_AFTER_LAUNCH  (1u << 1)

typedef struct {
    // Kernel ID
    uint64_t kernel_id;
    // Coming from Kernel Launch command
    uint64_t code_start_address;
    uint64_t pointer_to_args;
    uint64_t shire_mask;
    // Flags
    uint64_t kernel_launch_flags;
} __attribute__((aligned(64))) kernel_config_t;

#endif
