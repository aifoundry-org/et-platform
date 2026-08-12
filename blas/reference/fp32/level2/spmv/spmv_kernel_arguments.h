#ifndef ET_BLAS_REFERENCE_FP32_SPMV_KERNEL_ARGUMENTS_H
#define ET_BLAS_REFERENCE_FP32_SPMV_KERNEL_ARGUMENTS_H

#include <cstdint>

struct KernelArguments {
  char uplo;
  int32_t n;
  float alpha;
  const float* ap;
  const float* x;
  int32_t incx;
  float beta;
  float* y;
  int32_t incy;
};

#endif
