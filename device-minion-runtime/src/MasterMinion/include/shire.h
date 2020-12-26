#ifndef SHIRE_H
#define SHIRE_H

#include "kernel.h"

#include <stdbool.h>
#include <stdint.h>

#define M_SHIRE_BASE_HART_ID               2048
#define M_SHIRE_NUM_MINIONS                32
#define M_SHIRE_NUM_THREADS_PER_MINION     2

typedef enum {
    SHIRE_STATE_UNKNOWN = 0,
    SHIRE_STATE_READY,
    SHIRE_STATE_RUNNING,
    SHIRE_STATE_ERROR,
    SHIRE_STATE_COMPLETE
} shire_state_t;

void set_functional_shires(uint64_t mask);
uint64_t get_functional_shires(void);
void update_shire_state(uint64_t shire, shire_state_t state);
bool all_shires_booted(uint64_t shire_mask);
bool all_shires_ready(uint64_t shire_mask);
void set_shire_kernel_id(uint64_t shire, kernel_id_t kernel_id);
kernel_id_t get_shire_kernel_id(uint64_t shire);

#endif
