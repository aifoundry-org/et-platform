#ifndef KERNEL_H
#define KERNEL_H

#include "kernel_info.h"

#include <stdint.h>

#define KERNEL_LAUNCH_ERROR_ABORTED          -1000LL
#define KERNEL_LAUNCH_ERROR_NO_SAVED_CONTEXT -1001LL

int64_t launch_kernel(const uint64_t* const kernel_entry_addr,
                      const uint64_t* const kernel_stack_addr,
                      const kernel_params_t* const kernel_params_ptr,
                      const grid_config_t* const grid_config_ptr);

void kernel_function(void);
int64_t return_from_kernel(int64_t return_value);

#endif
