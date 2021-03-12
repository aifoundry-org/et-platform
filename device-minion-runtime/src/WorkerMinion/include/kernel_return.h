#ifndef KERNEL_RETURN_H
#define KERNEL_RETURN_H

#include <stdint.h>

void __attribute__((noreturn)) return_from_kernel(int64_t return_value);

#endif
