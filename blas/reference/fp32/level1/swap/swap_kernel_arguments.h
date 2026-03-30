#ifndef ET_BLAS_REFERENCE_FP32_SWAP_KERNEL_ARGUMENTS_H
#define ET_BLAS_REFERENCE_FP32_SWAP_KERNEL_ARGUMENTS_H

#include <cstdint>

struct KernelArguments {
  uint64_t numElements;
  float* x;
  float* y;
} __attribute__((packed));

#endif
