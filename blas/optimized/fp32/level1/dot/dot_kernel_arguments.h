#ifndef ET_BLAS_OPTIMIZED_FP32_DOT_KERNEL_ARGUMENTS_H
#define ET_BLAS_OPTIMIZED_FP32_DOT_KERNEL_ARGUMENTS_H

#include <cstdint>

// Keep this ABI aligned with the reference dot kernel.
struct KernelArguments {
  uint64_t numElements;
  const float* x;
  const float* y;
  float* partials;
  float* res;
} __attribute__((packed));

#endif
