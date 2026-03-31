#ifndef ET_BLAS_REFERENCE_FP32_GER_KERNEL_ARGUMENTS_H
#define ET_BLAS_REFERENCE_FP32_GER_KERNEL_ARGUMENTS_H

#include <cstdint>

struct KernelArguments {
  int32_t m;
  int32_t n;
  float alpha;
  const float* x;
  int32_t incx;
  const float* y;
  int32_t incy;
  float* a;
  int32_t lda;
};

#endif
