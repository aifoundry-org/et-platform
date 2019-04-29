#include "eti.h"
#include "../kernels/kernel_args.h"
#include "EsperantoRuntime.h"
#include "Support/DeviceGuard.h"
#include <assert.h>
#include <math.h>
#include <stdlib.h>

#define INCLUDE_FOR_HOST
#include "../kernels/sys_inc.h"
#undef INCLUDE_FOR_HOST

/*
 * C = op(A) * op(B)
 *
 * A, B, C - in row-major
 *
 * dimensions (after transposition):
 * op(A) - m x k
 * op(B) - k x n
 * C     - m x n
 *
 * ld[abc] - leading dimension of a two-dimensional array used to store the
 * matrix [ABC] ld[abc] may be bigger than corresponding matrix dimension, it
 * could be useful in cases when matrix rows/columns lies in memory in
 * non-consecutive manner (sub-matrix, for example)
 */
void etiMatMulTensor(etrtStream_t stream, int m, int n, int k, const float *A,
                     int lda, bool is_transa, const float *B, int ldb,
                     bool is_transb, float *C, int ldc) {
  {
    GetDev dev;

    assert(dev->isPtrAllocedDev(A));
    assert(dev->isPtrAllocedDev(B));
    assert(dev->isPtrAllocedDev(C));
  }

  /*
   * Esperanto tensor instructions have some restrictions.
   * Note also that Esperanto tensor instructions have internally row-major
   * layout;
   */
  assert(is_aligned((uintptr_t)A, 64));
  assert(is_aligned((uintptr_t)B, 64));
  assert(is_aligned((uintptr_t)C, 64));
  assert(is_aligned(lda * sizeof(float), 64));
  assert(is_aligned(ldb * sizeof(float), 64));
  assert(is_aligned(ldc * sizeof(float), 64));
  // assert((n % 4) == 0);

  // each work item will compute 16x16 block of result matrix
  dim3 gridDim(
      defaultGridDim1D((align_up(m, 16) / 16) * (align_up(n, 16) / 16)));
  dim3 blockDim(defaultBlockDim1D());
  etrtConfigureCall(gridDim, blockDim, 0, stream);

  MatMulTensor_KernelArgs_t args_struct = {m, n,   k,         A, lda, is_transa,
                                           B, ldb, is_transb, C, ldc};
  etrtSetupArgument(&args_struct, sizeof(args_struct), 0);
  etrtLaunch(nullptr, "MatMulTensor");
}

void etiMatMulTensorInt8To32(etrtStream_t stream, int m, int n, int k,
                             const int8_t *A, int lda, bool is_transa,
                             const int8_t *B, int ldb, bool is_transb,
                             int32_t *C, int ldc) {
  {
    GetDev dev;

    assert(dev->isPtrAllocedDev(A));
    assert(dev->isPtrAllocedDev(B));
    assert(dev->isPtrAllocedDev(C));
  }

  /*
   * Esperanto tensor instructions have some restrictions.
   * Note also that Esperanto tensor instructions have internally row-major
   * layout;
   */
  assert((k % 4) == 0);
  assert(is_aligned((uintptr_t)A, 64));
  assert(is_aligned((uintptr_t)B, 64));
  assert(is_aligned((uintptr_t)C, 64));
  assert(is_aligned(lda * sizeof(int8_t), 64));
  assert(is_aligned(ldb * sizeof(int8_t), 64));
  assert(is_aligned(ldc * sizeof(int32_t), 64));
  // assert((n % 4) == 0);

  // each work item will compute 16x16 block of result matrix
  dim3 gridDim(
      defaultGridDim1D((align_up(m, 16) / 16) * (align_up(n, 16) / 16)));
  dim3 blockDim(defaultBlockDim1D());
  etrtConfigureCall(gridDim, blockDim, 0, stream);

  MatMulTensorInt8To32_KernelArgs_t args_struct = {
      m, n, k, A, lda, is_transa, B, ldb, is_transb, C, ldc};
  etrtSetupArgument(&args_struct, sizeof(args_struct), 0);
  etrtLaunch(nullptr, "MatMulTensorInt8To32");
}

void etiBatchedAdd(etrtStream_t stream, int n, int slice_size,
                   const float *batch, const float *slice, float *dest) {
  {
    GetDev dev;

    assert(dev->isPtrAllocedDev(batch));
    assert(dev->isPtrAllocedDev(slice));
    assert(dev->isPtrAllocedDev(dest));
  }

  dim3 gridDim(defaultGridDim1D(n * slice_size));
  dim3 blockDim(defaultBlockDim1D());
  etrtConfigureCall(gridDim, blockDim, 0, stream);

  BatchedAdd_KernelArgs_t args_struct = {n, slice_size, batch, slice, dest};
  etrtSetupArgument(&args_struct, sizeof(args_struct), 0);
  etrtLaunch(nullptr, "BatchedAdd");
}

void etiQuantizedBatchedAdd(etrtStream_t stream, int n, int slice_size,
                            const int8_t *batch, float batchScale,
                            int32_t batchOffset, const int8_t *slice,
                            float sliceScale, int32_t sliceOffset, int8_t *dest,
                            float destScale, int32_t destOffset) {
  {
    GetDev dev;

    assert(dev->isPtrAllocedDev(batch));
    assert(dev->isPtrAllocedDev(slice));
    assert(dev->isPtrAllocedDev(dest));
  }

  dim3 gridDim(defaultGridDim1D(n * slice_size));
  dim3 blockDim(defaultBlockDim1D());
  etrtConfigureCall(gridDim, blockDim, 0, stream);

  QuantizedBatchedAdd_KernelArgs_t args_struct = {
      n,          slice_size,  batch, batchScale, batchOffset, slice,
      sliceScale, sliceOffset, dest,  destScale,  destOffset};
  etrtSetupArgument(&args_struct, sizeof(args_struct), 0);
  etrtLaunch(nullptr, "QuantizedBatchedAdd");
}
