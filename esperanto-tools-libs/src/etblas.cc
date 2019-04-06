#include "etblas.h"
#include "etblas-bin.h"
#include "../kernels/kernel_args.h"
#include "C-API/etrt.h"
#include "et_device.h"
#include "eti.h"
#include <assert.h>
#include <stdlib.h>

struct EtBlasContext {
  etblasPointerMode_t pointer_mode = ETBLAS_POINTER_MODE_HOST;
  EtStream *etStream = nullptr;
};

EXAPI etblasStatus_t etblasCreate_v2(etblasHandle_t *handle) {
  EtBlasContext *ctx = new EtBlasContext();
  *handle = (etblasHandle_t)ctx;
  return ETBLAS_STATUS_SUCCESS;
}

EXAPI etblasStatus_t etblasDestroy_v2(etblasHandle_t handle) {
  EtBlasContext *ctx = (EtBlasContext *)handle;
  delete ctx;
  return ETBLAS_STATUS_SUCCESS;
}

EXAPI etblasStatus_t etblasSetPointerMode_v2(etblasHandle_t handle,
                                             etblasPointerMode_t mode) {
  EtBlasContext *ctx = (EtBlasContext *)handle;
  ctx->pointer_mode = mode;
  return ETBLAS_STATUS_SUCCESS;
}

EXAPI etblasStatus_t etblasSetStream_v2(etblasHandle_t handle,
                                        etrtStream_t streamId) {
  EtBlasContext *ctx = (EtBlasContext *)handle;

  GetDev dev;

  ctx->etStream = dev->getStream(streamId);
  return ETBLAS_STATUS_SUCCESS;
}

/*
 * C = α * op(A) * op(B) + β * C
 *
 * A, B, C - in column-major format (as in Fortran, i.e. first index (=index of
 * row) is dense)
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
 *
 * See also: cpuSgemm_v2
 */
EXAPI etblasStatus_t etblasSgemm_v2(
    etblasHandle_t handle, etblasOperation_t transa, etblasOperation_t transb,
    int m, int n, int k, const float *alpha, /* host or device pointer */
    const float *A, int lda, const float *B, int ldb,
    const float *beta, /* host or device pointer */
    float *C, int ldc) {
  EtBlasContext *ctx = (EtBlasContext *)handle;

  assert(ctx->pointer_mode == ETBLAS_POINTER_MODE_HOST);
  float alpha_val = *alpha;
  float beta_val = *beta;

  {
    GetDev dev;

    assert(dev->isPtrAllocedDev(A));
    assert(dev->isPtrAllocedDev(B));
    assert(dev->isPtrAllocedDev(C));
  }

  // no sense in conjugate transpose for float data
  assert(transa == ETBLAS_OP_N || transa == ETBLAS_OP_T);
  assert(transb == ETBLAS_OP_N || transb == ETBLAS_OP_T);

  /*
   * Try to implement matmul using tensor instructions.
   * There are some restrictions for such use.
   */
  if (alpha_val == 1.0f && beta_val == 0.0f && is_aligned((uintptr_t)A, 64) &&
      is_aligned((uintptr_t)B, 64) && is_aligned((uintptr_t)C, 64) &&
      is_aligned(lda * sizeof(float), 64) &&
      is_aligned(ldb * sizeof(float), 64) && is_aligned(ldc * sizeof(float), 64)
      /*&& (m % 4) == 0*/) {
    /*
     * Tensor instructions expect matrices in row-major format, so we use
     * mathematical identity: if C = A * B then Cᵀ = Bᵀ * Aᵀ
     */
    etiMatMulTensor(reinterpret_cast<etrtStream_t>(ctx->etStream), n, m, k, B,
                    ldb, transb == ETBLAS_OP_T, A, lda, transa == ETBLAS_OP_T,
                    C, ldc);
    return ETBLAS_STATUS_SUCCESS;
  }

  dim3 gridDim(defaultGridDim1D(m * n));
  dim3 blockDim(defaultBlockDim1D());
  etrtConfigureCall(gridDim, blockDim, 0,
                    reinterpret_cast<etrtStream_t>(ctx->etStream));

  Sgemm_v2_KernelArgs_t args_struct = {
      transa, transb, m, n, k, alpha_val, beta_val, lda, ldb, ldc, A, B, C};

  etrtSetupArgument(&args_struct, sizeof(args_struct), 0);

  etrtLaunch(nullptr, "Sgemm_v2");

  return ETBLAS_STATUS_SUCCESS;
}

EXAPI etblasStatus_t etblasGather(etblasHandle_t handle, int elem_size,
                                  int64_t *indices, int indices_n, void *data,
                                  int data_xdim, int data_ydim, void *dest,
                                  int dest_xdim) {
  EtBlasContext *ctx = (EtBlasContext *)handle;

  {
    GetDev dev;

    assert(dev->isPtrAllocedDev(indices));
    assert(dev->isPtrAllocedDev(data));
    assert(dev->isPtrAllocedDev(dest));
  }

  dim3 gridDim(defaultGridDim1D(indices_n));
  dim3 blockDim(defaultBlockDim1D());
  etrtConfigureCall(gridDim, blockDim, 0,
                    reinterpret_cast<etrtStream_t>(ctx->etStream));

  Gather_KernelArgs_t args_struct{elem_size, indices,   indices_n, data,
                                  data_xdim, data_ydim, dest,      dest_xdim};

  etrtSetupArgument(&args_struct, sizeof(args_struct), 0);

  if (elem_size == 4) {
    etrtLaunch(nullptr, "Gather_Float");
  } else {
    abort();
  }

  return ETBLAS_STATUS_SUCCESS;
}

EXAPI etblasStatus_t etblasTopK(etblasHandle_t handle, void *in, int in_sz,
                                void *indices, int k, void *out) {
  EtBlasContext *ctx = (EtBlasContext *)handle;

  {
    GetDev dev;

    assert(dev->isPtrAllocedDev(in));
    assert(dev->isPtrAllocedDev(indices));
    assert(dev->isPtrAllocedDev(out));
  }

  {
    etrtStream_t stream = reinterpret_cast<etrtStream_t>(ctx->etStream);

    float *in_host = new float[in_sz];
    size_t *inidx = new size_t[in_sz];
    float *out_host = new float[k];
    size_t *idx_host = new size_t[k];

    assert(in_sz > k);

    etrtMemcpyAsync(in_host, in, in_sz * sizeof(float), etrtMemcpyDeviceToHost,
                    stream);
    etrtStreamSynchronize(stream);

    for (int i = 0; i < in_sz; i++) {
      inidx[i] = i;
    }

    for (int i = 0; i < k; i++) {
      float m = in_host[0];
      size_t m_j = 0;
      size_t m_rj = 0;
      for (int j = 1; j < (in_sz - i); j++) {
        if (in_host[j] > m) {
          m = in_host[j];
          m_j = j;
          m_rj = inidx[j];
        }
      }
      out_host[i] = m;
      idx_host[i] = m_rj;
      in_host[m_j] = in_host[in_sz - i - 1];
      inidx[m_j] = inidx[in_sz - i - 1];
    }

    etrtMemcpyAsync(out, out_host, k * sizeof(float), etrtMemcpyHostToDevice,
                    stream);
    etrtMemcpyAsync(indices, idx_host, k * sizeof(size_t),
                    etrtMemcpyHostToDevice, stream);
    etrtStreamSynchronize(stream);

    delete[] in_host;
    delete[] inidx;
    delete[] out_host;
    delete[] idx_host;
  }

#if 0
    dim3 gridDim(defaultGridDim1D(1)); // FIXME:
    dim3 blockDim(defaultBlockDim1D());
    etrtConfigureCall(gridDim, blockDim, 0, reinterpret_cast<etrtStream_t>(ctx->etStream));

    TopK_KernelArgs_t args_struct;
    args_struct.in = in;
    args_struct.in_sz = in_sz;
    args_struct.indices_out = (size_t*)indices;
    args_struct.k = k;
    args_struct.out = out;

    etrtSetupArgument(&args_struct, sizeof(args_struct), 0);

    etrtLaunch(nullptr, "TopK_Float");
#endif

  return ETBLAS_STATUS_SUCCESS;
}

EXAPI etblasStatus_t etblasLog(etblasHandle_t handle, float *in, float *out,
                               int size) {
  EtBlasContext *ctx = (EtBlasContext *)handle;

  {
    GetDev dev;

    assert(dev->isPtrAllocedDev(in));
    assert(dev->isPtrAllocedDev(out));
  }

  dim3 gridDim(defaultGridDim1D(size));
  dim3 blockDim(defaultBlockDim1D());
  etrtConfigureCall(gridDim, blockDim, 0,
                    reinterpret_cast<etrtStream_t>(ctx->etStream));

  etrtSetupArgument(&size, 4, 0);
  etrtSetupArgument(&in, 8, 8);
  etrtSetupArgument(&out, 8, 16);
  etrtLaunch(nullptr, "LogKernel_Float");

  return ETBLAS_STATUS_SUCCESS;
}

EXAPI etblasStatus_t etblasQuantize(etblasHandle_t handle, float *in,
                                    int8_t *out, int size, float scale,
                                    int32_t offset) {
  EtBlasContext *ctx = (EtBlasContext *)handle;

  {
    GetDev dev;

    assert(dev->isPtrAllocedDev(in));
    assert(dev->isPtrAllocedDev(out));
  }

  dim3 gridDim(defaultGridDim1D(size));
  dim3 blockDim(defaultBlockDim1D());
  etrtConfigureCall(gridDim, blockDim, 0,
                    reinterpret_cast<etrtStream_t>(ctx->etStream));

  etrtSetupArgument(&size, 4, 0);
  etrtSetupArgument(&in, 8, 8);
  etrtSetupArgument(&out, 8, 16);
  etrtSetupArgument(&scale, 4, 24);
  etrtSetupArgument(&offset, 4, 28);
  etrtLaunch(nullptr, "QuantizeKernel");

  return ETBLAS_STATUS_SUCCESS;
}

EXAPI etblasStatus_t etblasDequantize(etblasHandle_t handle, int8_t *in,
                                      float *out, int size, float scale,
                                      int32_t offset) {
  EtBlasContext *ctx = (EtBlasContext *)handle;

  {
    GetDev dev;

    assert(dev->isPtrAllocedDev(in));
    assert(dev->isPtrAllocedDev(out));
  }

  dim3 gridDim(defaultGridDim1D(size));
  dim3 blockDim(defaultBlockDim1D());
  etrtConfigureCall(gridDim, blockDim, 0,
                    reinterpret_cast<etrtStream_t>(ctx->etStream));

  etrtSetupArgument(&size, 4, 0);
  etrtSetupArgument(&in, 8, 8);
  etrtSetupArgument(&out, 8, 16);
  etrtSetupArgument(&scale, 4, 24);
  etrtSetupArgument(&offset, 4, 28);
  etrtLaunch(nullptr, "DequantizeKernel");

  return ETBLAS_STATUS_SUCCESS;
}

EXAPI etblasStatus_t etblasRescaleQuantized(etblasHandle_t handle, int8_t *in,
                                            int8_t *out, int size,
                                            float inScale, int32_t inOffset,
                                            float outScale, int32_t outOffset) {
  EtBlasContext *ctx = (EtBlasContext *)handle;

  {
    GetDev dev;

    assert(dev->isPtrAllocedDev(in));
    assert(dev->isPtrAllocedDev(out));
  }

  dim3 gridDim(defaultGridDim1D(size));
  dim3 blockDim(defaultBlockDim1D());
  etrtConfigureCall(gridDim, blockDim, 0,
                    reinterpret_cast<etrtStream_t>(ctx->etStream));

  etrtSetupArgument(&size, 4, 0);
  etrtSetupArgument(&in, 8, 8);
  etrtSetupArgument(&out, 8, 16);
  etrtSetupArgument(&inScale, 4, 24);
  etrtSetupArgument(&inOffset, 4, 28);
  etrtSetupArgument(&outScale, 4, 32);
  etrtSetupArgument(&outOffset, 4, 36);
  etrtLaunch(nullptr, "RescaleQuantizedKernel");

  return ETBLAS_STATUS_SUCCESS;
}

EXAPI etblasStatus_t etblasQuantizedMatMul(
    etblasHandle_t handle, int m, int n, int k, const int8_t *A, int lda,
    bool is_transa, int32_t offseta, const int8_t *B, int ldb, bool is_transb,
    int32_t offsetb, int8_t *C, int ldc, int32_t offsetc, float invScale) {
  EtBlasContext *ctx = (EtBlasContext *)handle;

  {
    GetDev dev;

    assert(dev->isPtrAllocedDev(A));
    assert(dev->isPtrAllocedDev(B));
    assert(dev->isPtrAllocedDev(C));
  }

  dim3 gridDim(defaultGridDim1D(m * n));
  dim3 blockDim(defaultBlockDim1D());
  etrtConfigureCall(gridDim, blockDim, 0,
                    reinterpret_cast<etrtStream_t>(ctx->etStream));

  QuantizedMatMul_KernelArgs_t args_struct = {
      m,   n,         k,       A, lda, is_transa, offseta, B,
      ldb, is_transb, offsetb, C, ldc, offsetc,   invScale};
  etrtSetupArgument(&args_struct, sizeof(args_struct), 0);
  etrtLaunch(nullptr, "QuantizedMatMul");

  return ETBLAS_STATUS_SUCCESS;
}

EXAPI etblasStatus_t etblasQuantizedBatchedAdd(
    etblasHandle_t handle, int n, int slice_size, const int8_t *batch,
    float batchScale, int32_t batchOffset, const int8_t *slice,
    float sliceScale, int32_t sliceOffset, int8_t *dest, float destScale,
    int32_t destOffset) {
  EtBlasContext *ctx = (EtBlasContext *)handle;

  etiQuantizedBatchedAdd(reinterpret_cast<etrtStream_t>(ctx->etStream), n,
                         slice_size, batch, batchScale, batchOffset, slice,
                         sliceScale, sliceOffset, dest, destScale, destOffset);

  return ETBLAS_STATUS_SUCCESS;
}

EXAPI etblasStatus_t etblasBatchedAdd(etblasHandle_t handle, int n,
                                      int slice_size, const float *batch,
                                      const float *slice, float *dest) {
  EtBlasContext *ctx = (EtBlasContext *)handle;

  etiBatchedAdd(reinterpret_cast<etrtStream_t>(ctx->etStream), n, slice_size,
                batch, slice, dest);

  return ETBLAS_STATUS_SUCCESS;
}
