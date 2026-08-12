#ifndef ET_BLAS_REFERENCE_FP32_GBMV_KERNEL_ARGUMENTS_H
#define ET_BLAS_REFERENCE_FP32_GBMV_KERNEL_ARGUMENTS_H

#include <cstdint>

struct KernelArguments {
  char trans;
  int32_t m;
  int32_t n;
  int32_t kl;
  int32_t ku;
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
