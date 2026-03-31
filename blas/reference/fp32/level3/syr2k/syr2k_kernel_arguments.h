#ifndef ET_BLAS_REFERENCE_FP32_SYR2K_KERNEL_ARGUMENTS_H
#define ET_BLAS_REFERENCE_FP32_SYR2K_KERNEL_ARGUMENTS_H

#include <cstdint>

struct KernelArguments {
  char uplo;
  char trans;
  int32_t n;
  int32_t k;
  float alpha;
  const float* a;
  int32_t lda;
  const float* b;
  int32_t ldb;
  float beta;
  float* c;
  int32_t ldc;
};

#endif
