#ifndef KERNEL_H
#define KERNEL_H

#include "kernel_info.h"
#include "kernel_sync.h"

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    KERNEL_ID_0 = 0,
    KERNEL_ID_1,
    KERNEL_ID_2,
    KERNEL_ID_3,
    KERNEL_ID_NONE
} kernel_id_t;

typedef enum
{
    KERNEL_STATE_UNUSED = 0,
    KERNEL_STATE_LAUNCHED,
    KERNEL_STATE_RUNNING,
    KERNEL_STATE_ABORTED,
    KERNEL_STATE_ERROR,
    KERNEL_STATE_COMPLETE
} kernel_state_t;

void kernel_init(void);
void __attribute__((noreturn)) kernel_sync_thread(uint64_t kernel_id);
void update_kernel_state(kernel_id_t kernel_id, kernel_state_t kernel_state);
void launch_kernel(const kernel_params_t* const kernel_params_ptr, const kernel_info_t* const kernel_info_ptr);
void abort_kernel(kernel_id_t kernel_id);
kernel_state_t get_kernel_state(kernel_id_t kernel_id);

#endif
