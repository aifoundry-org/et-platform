#ifndef ET_BLAS_REFERENCE_FP32_COPY_KERNEL_ARGUMENTS_H
#define ET_BLAS_REFERENCE_FP32_COPY_KERNEL_ARGUMENTS_H

#include <cstdint>

struct KernelArguments {
  uint64_t numElements;
  const float* x;
  float* y;
} __attribute__((packed));

#endif
