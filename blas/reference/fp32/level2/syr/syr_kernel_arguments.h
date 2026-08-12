#ifndef ET_BLAS_REFERENCE_FP32_SYR_KERNEL_ARGUMENTS_H
#define ET_BLAS_REFERENCE_FP32_SYR_KERNEL_ARGUMENTS_H

#include <cstdint>

struct KernelArguments {
  char uplo;
  int32_t n;
  float alpha;
  const float* x;
  int32_t incx;
  float* a;
  int32_t lda;
};

#endif
