#ifndef ET_BLAS_REFERENCE_FP32_TRMM_KERNEL_ARGUMENTS_H
#define ET_BLAS_REFERENCE_FP32_TRMM_KERNEL_ARGUMENTS_H

#include <cstdint>

struct KernelArguments {
  char side;
  char uplo;
  char transa;
  char diag;
  int32_t m;
  int32_t n;
  float alpha;
  const float* a;
  int32_t lda;
  float* b;
  int32_t ldb;
};

#endif
