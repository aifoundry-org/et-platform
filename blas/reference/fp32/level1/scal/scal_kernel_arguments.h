#ifndef ET_BLAS_REFERENCE_FP32_SCAL_KERNEL_ARGUMENTS_H
#define ET_BLAS_REFERENCE_FP32_SCAL_KERNEL_ARGUMENTS_H

#include <cstdint>

struct KernelArguments {
  uint64_t numElements;
  float* x;
  float alpha;
} __attribute__((packed));

#endif
