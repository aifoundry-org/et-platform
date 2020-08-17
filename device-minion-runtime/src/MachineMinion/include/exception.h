#ifndef EXCEPTION_H
#define EXCEPTION_H

#include <stdint.h>

void exception_handler(uint64_t mcause, uint64_t mepc, uint64_t mtval, uint64_t *const reg);
int64_t register_return_from_kernel_function(int64_t (*function_ptr)(int64_t));

#endif
