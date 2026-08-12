#ifndef ET_BLAS_REFERENCE_FP32_SBMV_KERNEL_ARGUMENTS_H
#define ET_BLAS_REFERENCE_FP32_SBMV_KERNEL_ARGUMENTS_H

#include <cstdint>

struct KernelArguments {
  char uplo;
  int32_t n;
  int32_t k;
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
