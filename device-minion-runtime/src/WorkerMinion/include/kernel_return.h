#ifndef KERNEL_RETURN_H
#define KERNEL_RETURN_H

#include <stdint.h>

/* Kernel return types */
#define KERNEL_RETURN_SUCCESS      0
#define KERNEL_RETURN_SELF_ABORT   1
#define KERNEL_RETURN_SYSTEM_ABORT 2
#define KERNEL_RETURN_EXCEPTION    3
#define KERNEL_RETURN_BUS_ERROR    4

/* Function prototypes */
int64_t return_from_kernel(int64_t return_value, uint64_t return_type);
void kernel_self_abort_save_context(uint64_t stack_frame);

#endif
