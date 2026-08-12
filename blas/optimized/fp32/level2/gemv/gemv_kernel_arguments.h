#ifndef ET_BLAS_OPTIMIZED_FP32_GEMV_KERNEL_ARGUMENTS_H
#define ET_BLAS_OPTIMIZED_FP32_GEMV_KERNEL_ARGUMENTS_H

#include <cstdint>

struct KernelArguments {
  char trans;
  int32_t m;
  int32_t n;
  float alpha;
  const float* a;
  int32_t lda;
  const float* x;
  int32_t incx;
  float beta;
  float* y;
  int32_t incy;
};

#endif
