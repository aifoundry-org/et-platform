#ifndef WORKER_H
#define WORKER_H

#include <stdint.h>

int64_t launch_kernel(const uint64_t* const kernel_entry_addr, const uint64_t* const kernel_stack_addr, const uint64_t* const argument_ptr, const uint64_t* const grid_ptr) __attribute__ ((used));
void kernel_function(void) __attribute__ ((used));
int64_t return_from_kernel(void);

#endif
