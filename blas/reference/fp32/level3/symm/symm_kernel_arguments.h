#ifndef ET_BLAS_REFERENCE_FP32_SYMM_KERNEL_ARGUMENTS_H
#define ET_BLAS_REFERENCE_FP32_SYMM_KERNEL_ARGUMENTS_H

#include <cstdint>

struct KernelArguments {
  char side;
  char uplo;
  int32_t m;
  int32_t n;
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
