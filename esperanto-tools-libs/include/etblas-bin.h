#ifndef ETBLAS_BIN_H
#define ETBLAS_BIN_H

#include "etrt-bin.h"

/**
 * indicates which part (lower or upper) of the dense matrix was filled and
 * consequently should be used by the function. Its values correspond to Fortran
 * characters ‘L’ or ‘l’ (lower) and ‘U’ or ‘u’ (upper) that are often used as
 * parameters to legacy BLAS implementations.
 */
typedef enum {
  ETBLAS_FILL_MODE_LOWER = 0,
  ETBLAS_FILL_MODE_UPPER = 1
} etblasFillMode_t;

/**
 * indicates whether the main diagonal of the dense matrix is unity and
 * consequently should not be touched or modified by the function. Its values
 * correspond to Fortran characters ‘N’ or ‘n’ (non-unit) and ‘U’ or ‘u’ (unit)
 * that are often used as parameters to legacy BLAS implementations.
 */
typedef enum {
  ETBLAS_DIAG_NON_UNIT = 0,
  ETBLAS_DIAG_UNIT = 1
} etblasDiagType_t;

/**
 * indicates whether the dense matrix is on the left or right side in the matrix
 * equation solved by a particular function. Its values correspond to Fortran
 * characters ‘L’ or ‘l’ (left) and ‘R’ or ‘r’ (right) that are often used as
 * parameters to legacy BLAS implementations.
 */
typedef enum { ETBLAS_SIDE_LEFT = 0, ETBLAS_SIDE_RIGHT = 1 } etblasSideMode_t;

typedef enum {
  ETBLAS_ATOMICS_NOT_ALLOWED = 0,
  ETBLAS_ATOMICS_ALLOWED = 1
} etblasAtomicsMode_t;

/* For different GEMM algorithm */
typedef enum {
  ETBLAS_GEMM_DFALT = -1,
  ETBLAS_GEMM_DEFAULT = -1,
  ETBLAS_GEMM_ALGO0 = 0,
  ETBLAS_GEMM_ALGO1 = 1,
  ETBLAS_GEMM_ALGO2 = 2,
  ETBLAS_GEMM_ALGO3 = 3,
  ETBLAS_GEMM_ALGO4 = 4,
  ETBLAS_GEMM_ALGO5 = 5,
  ETBLAS_GEMM_ALGO6 = 6,
  ETBLAS_GEMM_ALGO7 = 7,
  ETBLAS_GEMM_ALGO8 = 8,
  ETBLAS_GEMM_ALGO9 = 9,
  ETBLAS_GEMM_ALGO10 = 10,
  ETBLAS_GEMM_ALGO11 = 11,
  ETBLAS_GEMM_ALGO12 = 12,
  ETBLAS_GEMM_ALGO13 = 13,
  ETBLAS_GEMM_ALGO14 = 14,
  ETBLAS_GEMM_ALGO15 = 15,
  ETBLAS_GEMM_ALGO16 = 16,
  ETBLAS_GEMM_ALGO17 = 17,
  ETBLAS_GEMM_DEFAULT_TENSOR_OP = 99,
  ETBLAS_GEMM_DFALT_TENSOR_OP = 99,
  ETBLAS_GEMM_ALGO0_TENSOR_OP = 100,
  ETBLAS_GEMM_ALGO1_TENSOR_OP = 101,
  ETBLAS_GEMM_ALGO2_TENSOR_OP = 102,
  ETBLAS_GEMM_ALGO3_TENSOR_OP = 103,
  ETBLAS_GEMM_ALGO4_TENSOR_OP = 104
} etblasGemmAlgo_t;

/* Enum for default math mode/tensor operation */
typedef enum {
  ETBLAS_DEFAULT_MATH = 0,
  ETBLAS_TENSOR_OP_MATH = 1
} etblasMath_t;

/* For backward compatibility purposes */
typedef etrtDataType etblasDataType_t;


/****** ******/

#endif // ETBLAS_BIN_H
