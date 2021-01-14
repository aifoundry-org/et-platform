#ifndef KERNEL_H
#define KERNEL_H

#include "kernel_error.h"
#include "kernel_return.h"

#include <stdint.h>

int64_t launch_kernel(uint64_t kernel_id,
                      const uint64_t *const kernel_entry_addr,
                      const uint64_t *const kernel_stack_addr,
                      const uint64_t *const kernel_params_ptr,
                      uint64_t kernel_launch_flags);

void kernel_function(void);

#endif
