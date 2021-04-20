#ifndef KERNEL_H
#define KERNEL_H

#include "kernel_error.h"
#include "kernel_return.h"

#include <stdint.h>
#include <stdbool.h>

bool kernel_info_has_thread_completed(uint32_t shire_id, uint64_t thread_id);
void kernel_info_get_attributes(uint32_t shire_id, uint8_t *kw_base_id, uint8_t *slot_index);
uint64_t kernel_info_set_thread_returned(uint32_t shire_id, uint64_t thread_id);

void kernel_launch_post_cleanup(uint8_t kw_base_id, uint8_t slot_index, int64_t kernel_ret_val);

int64_t launch_kernel(uint8_t kw_base_id,
                      uint8_t slot_index,
                      uint64_t kernel_entry_addr,
                      uint64_t kernel_stack_addr,
                      uint64_t kernel_params_ptr,
                      uint64_t kernel_launch_flags,
                      uint64_t kernel_shire_mask);

#endif
