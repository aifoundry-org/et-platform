#ifndef KERNEL_H
#define KERNEL_H

#include "kernel_error.h"
#include "kernel_return.h"

#include <stdint.h>

void kernel_info_get_attributes(uint32_t shire_id, uint8_t *kw_base_id, uint8_t *slot_index);

int64_t launch_kernel(uint8_t kw_base_id,
                      uint8_t slot_index,
                      uint64_t kernel_entry_addr,
                      uint64_t kernel_stack_addr,
                      uint64_t kernel_params_ptr,
                      uint64_t kernel_launch_flags,
                      uint64_t kernel_shire_mask);

#endif
