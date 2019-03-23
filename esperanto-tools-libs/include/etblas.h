#ifndef ETBLAS_H
#define ETBLAS_H

#include "et-misc.h"
#include "etblas-bin.h"
#include "etrt.h"
#include <stdint.h>

EXAPI etblasStatus_t etblasCreate_v2(etblasHandle_t *handle);
EXAPI etblasStatus_t etblasDestroy_v2(etblasHandle_t handle);
EXAPI etblasStatus_t etblasSetPointerMode_v2(etblasHandle_t handle,
                                             etblasPointerMode_t mode);
EXAPI etblasStatus_t etblasSetStream_v2(etblasHandle_t handle,
                                        etrtStream_t streamId);
EXAPI etblasStatus_t etblasSgemm_v2(etblasHandle_t handle,
                                    etblasOperation_t transa,
                                    etblasOperation_t transb, int m, int n,
                                    int k, const float *alpha, const float *A,
                                    int lda, const float *B, int ldb,
                                    const float *beta, float *C, int ldc);

EXAPI etblasStatus_t etblasGather(etblasHandle_t handle, int elem_size,
                                  int64_t *indices, int indices_n, void *data,
                                  int data_xdim, int data_ydim, void *dest,
                                  int dest_xdim);

EXAPI etblasStatus_t etblasTopK(etblasHandle_t handle, void *in, int in_sz,
                                void *indices, int k, void *out);

EXAPI etblasStatus_t etblasLog(etblasHandle_t handle, float *in, float *out,
                               int size);

EXAPI etblasStatus_t etblasQuantize(etblasHandle_t handle, float *in,
                                    int8_t *out, int size, float scale,
                                    int32_t offset);

EXAPI etblasStatus_t etblasDequantize(etblasHandle_t handle, int8_t *in,
                                      float *out, int size, float scale,
                                      int32_t offset);

EXAPI etblasStatus_t etblasRescaleQuantized(etblasHandle_t handle, int8_t *in,
                                            int8_t *out, int size,
                                            float inScale, int32_t inOffset,
                                            float outScale, int32_t outOffset);

EXAPI etblasStatus_t etblasQuantizedMatMul(
    etblasHandle_t handle, int m, int n, int k, const int8_t *A, int lda,
    bool is_transa, int32_t offseta, const int8_t *B, int ldb, bool is_transb,
    int32_t offsetb, int8_t *C, int ldc, int32_t offsetc, float invScale);

EXAPI etblasStatus_t etblasQuantizedBatchedAdd(
    etblasHandle_t handle, int n, int slice_size, const int8_t *batch,
    float batchScale, int32_t batchOffset, const int8_t *slice,
    float sliceScale, int32_t sliceOffset, int8_t *dest, float destScale,
    int32_t destOffset);

EXAPI etblasStatus_t etblasBatchedAdd(etblasHandle_t handle, int n,
                                      int slice_size, const float *batch,
                                      const float *slice, float *dest);

#endif // ETBLAS_H
