#ifndef SHIRE_H
#define SHIRE_H

#include "kernel.h"

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    SHIRE_STATE_UNKNOWN = 0,
    SHIRE_STATE_READY,
    SHIRE_STATE_RUNNING,
    SHIRE_STATE_ERROR,
    SHIRE_STATE_COMPLETE
} shire_state_t;

void update_shire_state(uint64_t shire, shire_state_t state);
bool all_shires_ready(uint64_t shire_mask);
bool all_shires_complete(uint64_t shire_mask);
void set_shire_kernel_id(uint64_t shire, kernel_id_t kernel_id);
kernel_id_t get_shire_kernel_id(uint64_t shire);

#endif
