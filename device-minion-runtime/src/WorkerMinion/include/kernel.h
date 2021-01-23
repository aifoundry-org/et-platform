#ifndef KERNEL_H
#define KERNEL_H

#include "kernel_error.h"
#include "kernel_return.h"

#include <stdint.h>

int64_t launch_kernel(uint64_t kw_base_id,
                      uint64_t kernel_id,
                      uint64_t kernel_entry_addr,
                      uint64_t kernel_stack_addr,
                      uint64_t kernel_params_ptr,
                      uint64_t kernel_launch_flags,
                      uint64_t kernel_shire_mask);

#endif
