#ifndef ETI_H
#define ETI_H

#include "etrt.h"
#include <memory>
#include <stddef.h>

void etiMatMulTensor(etrtStream_t stream, int m, int n, int k, const float *A,
                     int lda, bool is_transa, const float *B, int ldb,
                     bool is_transb, float *C, int ldc);

void etiMatMulTensorInt8To32(etrtStream_t stream, int m, int n, int k,
                             const int8_t *A, int lda, bool is_transa,
                             const int8_t *B, int ldb, bool is_transb,
                             int32_t *C, int ldc);

void etiBatchedAdd(etrtStream_t stream, int n, int slice_size,
                   const float *batch, const float *slice, float *dest);

void etiQuantizedBatchedAdd(etrtStream_t stream, int n, int slice_size,
                            const int8_t *batch, float batchScale,
                            int32_t batchOffset, const int8_t *slice,
                            float sliceScale, int32_t sliceOffset, int8_t *dest,
                            float destScale, int32_t destOffset);

#endif // ETI_H
