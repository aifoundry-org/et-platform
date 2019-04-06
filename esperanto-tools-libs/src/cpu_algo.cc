#include "cpu_algo.h"
#include "utils.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

template <typename T>
static void blendResult(T alpha_val, T result, T beta_val, T &dst) {
  // if β = 0 then dst does not have to be a valid input
  dst = alpha_val * result + (beta_val == (T)0 ? (T)0 : (beta_val * dst));
}

template <typename T>
static T &matEl(T *X, int ldx, etblasOperation_t op, int i, int j) {
  if (op == ETBLAS_OP_N) {
    return X[i + j * ldx];
  } else {
    assert(op == ETBLAS_OP_T);
    return X[j + i * ldx];
  }
}

// matrix element accessor when matrix in row-major format
template <typename T>
static T &matElRM(T *X, int ldx, bool is_transx, int i, int j) {
  if (!is_transx) {
    return X[j + i * ldx];
  } else {
    return X[i + j * ldx];
  }
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
 */
void cpuSgemm_v2(Sgemm_v2_KernelArgs_t *args_p) {
  assert(args_p->transa == ETBLAS_OP_N || args_p->transa == ETBLAS_OP_T);
  assert(args_p->transb == ETBLAS_OP_N || args_p->transb == ETBLAS_OP_T);

  for (int i = 0; i < args_p->m; i++) {
    for (int j = 0; j < args_p->n; j++) {
      float &cEl = matEl(args_p->C, args_p->ldc, ETBLAS_OP_N, i, j);
      float sum = 0.0f;
      for (int l = 0; l < args_p->k; l++) {
        sum += matEl(args_p->A, args_p->lda, args_p->transa, i, l) *
               matEl(args_p->B, args_p->ldb, args_p->transb, l, j);
      }
      blendResult(args_p->alpha_val, sum, args_p->beta_val, cEl);
    }
  }
}

void cpuMatMulTensor(MatMulTensor_KernelArgs_t *args_p) {
  for (int i = 0; i < args_p->m; i++) {
    for (int j = 0; j < args_p->n; j++) {
      float &cEl = matElRM(args_p->C, args_p->ldc, false, i, j);
      float sum = 0.0f;
      for (int l = 0; l < args_p->k; l++) {
        sum += matElRM(args_p->A, args_p->lda, args_p->is_transa, i, l) *
               matElRM(args_p->B, args_p->ldb, args_p->is_transb, l, j);
      }
      cEl = sum;
    }
  }
}

void cpuMatMulTensorInt8To32(MatMulTensorInt8To32_KernelArgs_t *args_p) {
  for (int i = 0; i < args_p->m; i++) {
    for (int j = 0; j < args_p->n; j++) {
      int32_t &cEl = matElRM(args_p->C, args_p->ldc, false, i, j);
      int32_t sum = 0;
      for (int l = 0; l < args_p->k; l++) {
        int32_t aEl = matElRM(args_p->A, args_p->lda, args_p->is_transa, i, l);
        int32_t bEl = matElRM(args_p->B, args_p->ldb, args_p->is_transb, l, j);
        sum += aEl * bEl;
      }
      cEl = sum;
    }
  }
}

void cpuQuantizedMatMul(QuantizedMatMul_KernelArgs_t *args_p) {
  for (int i = 0; i < args_p->m; i++) {
    for (int j = 0; j < args_p->n; j++) {
      int8_t &cEl = matElRM(args_p->C, args_p->ldc, false, i, j);
      int32_t sum = 0;
      for (int l = 0; l < args_p->k; l++) {
        int32_t aEl = matElRM(args_p->A, args_p->lda, args_p->is_transa, i, l);
        int32_t bEl = matElRM(args_p->B, args_p->ldb, args_p->is_transb, l, j);
        sum += (aEl - args_p->offseta) * (bEl - args_p->offsetb);
      }
      cEl = quantization::clip<int32_t, int8_t>(
          std::round(args_p->invScale * sum + args_p->offsetc));
    }
  }
}

/*
 * 2nd part of implementation of Convolution via matrix mul.
 * On the 1st step calculated a dot product of all channels of individual
 * "pixel" on all possible filter channels. On the 2nd step we should sum
 * filter_h*filter_w elements of tempory matrix from the 1st step.
 */
void cpuConvTailNHWC_Float(ConvTailNHWC_KernelArgs_t *args_p) {
  float *dest = (float *)args_p->y;
  const float *ws =
      (float *)
          args_p->workspace; // Workspace = intermediate matrix after MatMul
  const float *bias = (float *)args_p->b;

  int ldws = args_p->filter_h * args_p->filter_w * args_p->out_c;

  for (int n = 0; n < args_p->batch_size; n++) {
    for (int c = 0; c < args_p->out_c; c++) {
      for (int h = 0; h < args_p->out_h; h++) {
        for (int w = 0; w < args_p->out_w; w++) {
          int out_idx = c + w * args_p->out_c +
                        h * args_p->out_c * args_p->out_w +
                        n * args_p->out_c * args_p->out_w * args_p->out_h;

          int y = (h * args_p->u) - args_p->pad_h;
          int x = (w * args_p->v) - args_p->pad_w;

          // Apply 2d filter
          float conv_sum = 0.0f;
          for (int fh = 0; fh < args_p->filter_h; fh++) {
            for (int fw = 0; fw < args_p->filter_w; fw++) {
              int ih = y + fh * args_p->dilation_h;
              int iw = x + fw * args_p->dilation_w;

              if (ih >= 0 && iw >= 0 && ih < args_p->in_h &&
                  iw < args_p->in_w) {
                int ws_y = iw + args_p->in_w * (ih + args_p->in_h * n);
                int ws_x = fw + args_p->filter_w * (fh + args_p->filter_h * c);

                conv_sum += ws[ws_y * ldws + ws_x];
              }
            }
          }

          if (bias) {
            conv_sum += bias[c];
          }

          dest[out_idx] = conv_sum;
        }
      }
    }
  }
}

void cpuConvTailNHWC_Int8Q(ConvTailNHWC_KernelArgs_t *args_p) {
  int8_t *dest = (int8_t *)args_p->y;
  const int32_t *ws =
      (int32_t *)
          args_p->workspace; // Workspace = intermediate matrix after MatMul
  const int32_t *bias = (int32_t *)args_p->b;

  int ldws = args_p->filter_h * args_p->filter_w * args_p->out_c;

  // Calculate the scale of the values that come out of the matrix
  // multiplication part of the calculation.
  float matMulScale = args_p->inScale * args_p->filterScale;

  for (int n = 0; n < args_p->batch_size; n++) {
    for (int c = 0; c < args_p->out_c; c++) {
      for (int h = 0; h < args_p->out_h; h++) {
        for (int w = 0; w < args_p->out_w; w++) {
          int out_idx = c + w * args_p->out_c +
                        h * args_p->out_c * args_p->out_w +
                        n * args_p->out_c * args_p->out_w * args_p->out_h;

          int y = (h * args_p->u) - args_p->pad_h;
          int x = (w * args_p->v) - args_p->pad_w;

          // Apply 2d filter
          int32_t conv_sum = 0;
          for (int fh = 0; fh < args_p->filter_h; fh++) {
            for (int fw = 0; fw < args_p->filter_w; fw++) {
              int ih = y + fh * args_p->dilation_h;
              int iw = x + fw * args_p->dilation_w;

              if (ih >= 0 && iw >= 0 && ih < args_p->in_h &&
                  iw < args_p->in_w) {
                int ws_y = iw + args_p->in_w * (ih + args_p->in_h * n);
                int ws_x = fw + args_p->filter_w * (fh + args_p->filter_h * c);

                conv_sum += ws[ws_y * ldws + ws_x];
              }
            }
          }

          if (bias) {
            // Scale the bias to match the scale of the matrix multiplication.
            int32_t B = std::round(float(bias[c] - args_p->biasOffset) *
                                   (args_p->biasScale / matMulScale));
            conv_sum += B;
          }

          // Scale the result back to the expected destination scale.
          dest[out_idx] = quantization::clip<int32_t, int8_t>(
              std::round(float(conv_sum) * (matMulScale / args_p->outScale) +
                         args_p->outOffset));
        }
      }
    }
  }
}

template <typename T>
static T &tensor4dEl(T *tensor, int n, int c, int h, int w, int ns, int cs,
                     int hs, int ws) {
  return tensor[n * ns + c * cs + h * hs + w * ws];
}

void cpuConvolutionForwardFloat(ConvolutionForward_KernelArgs_t *args_p) {
#define ELEM_IN(_n, _c, _h, _w)                                                \
  tensor4dEl((const float *)args_p->x, _n, _c, _h, _w, args_p->in_nStride,     \
             args_p->in_cStride, args_p->in_hStride, args_p->in_wStride)
#define ELEM_FILTER1(_n, _c, _h, _w)                                           \
  tensor4dEl((const float *)args_p->w, _n, _c, _h, _w,                         \
             args_p->filter_w * args_p->filter_h * args_p->filter_c,           \
             args_p->filter_w * args_p->filter_h, args_p->filter_w, 1)
#define ELEM_FILTER2(_n, _c, _h, _w)                                           \
  tensor4dEl((const float *)args_p->w, _n, _c, _h, _w,                         \
             args_p->filter_w * args_p->filter_h * args_p->filter_c, 1,        \
             args_p->filter_c * args_p->filter_w, args_p->filter_c)
#define ELEM_OUT(_n, _c, _h, _w)                                               \
  tensor4dEl((float *)args_p->y, _n, _c, _h, _w, args_p->out_nStride,          \
             args_p->out_cStride, args_p->out_hStride, args_p->out_wStride)
  // Separate pass for every image
  for (int n = 0; n < args_p->in_n; n++) {
    // Every 3d filter (CHW dimensions) creates channel in output image
    for (int c = 0; c < args_p->filter_k; c++) {
      // For every element in 2d image
      for (int h = 0; h < args_p->out_h; h++) {
        for (int w = 0; w < args_p->out_w; w++) {
          int y = (h * args_p->u) - args_p->pad_h;
          int x = (w * args_p->v) - args_p->pad_w;

          // Apply 2d filter
          float conv_sum = 0.0f;
          for (int fh = 0; fh < args_p->filter_h; fh++) {
            for (int fw = 0; fw < args_p->filter_w; fw++) {
              int ih = y + fh * args_p->dilation_h;
              int iw = x + fw * args_p->dilation_w;

              if (ih >= 0 && iw >= 0 && ih < args_p->in_h &&
                  iw < args_p->in_w) {
                /**
                 * Each input channel is filtered by channel filter and added to
                 * resulting sum In the end all channels of input image folded
                 * into one
                 */
                for (int fc = 0; fc < args_p->filter_c; fc++) {
                  if (args_p->filter_format == ETDNN_TENSOR_NCHW) {
                    conv_sum +=
                        ELEM_IN(n, fc, ih, iw) * ELEM_FILTER1(c, fc, fh, fw);
                  } else {
                    conv_sum +=
                        ELEM_IN(n, fc, ih, iw) * ELEM_FILTER2(c, fc, fh, fw);
                  }
                }
              }
            }
          }

          if (args_p->b) {
            conv_sum += ((float *)args_p->b)[c];
          }

          float &oelem = ELEM_OUT(n, c, h, w);
          blendResult(args_p->alpha.f, conv_sum, args_p->beta.f, oelem);
        }
      }
    }
  }
#undef ELEM_OUT
#undef ELEM_FILTER2
#undef ELEM_FILTER1
#undef ELEM_IN
}

void cpuConvolutionForwardInt8Q(ConvolutionForward_KernelArgs_t *args_p) {
#define ELEM_IN(_n, _c, _h, _w)                                                \
  tensor4dEl((const int8_t *)args_p->x, _n, _c, _h, _w, args_p->in_nStride,    \
             args_p->in_cStride, args_p->in_hStride, args_p->in_wStride)
#define ELEM_FILTER1(_n, _c, _h, _w)                                           \
  tensor4dEl((const int8_t *)args_p->w, _n, _c, _h, _w,                        \
             args_p->filter_w * args_p->filter_h * args_p->filter_c,           \
             args_p->filter_w * args_p->filter_h, args_p->filter_w, 1)
#define ELEM_FILTER2(_n, _c, _h, _w)                                           \
  tensor4dEl((const int8_t *)args_p->w, _n, _c, _h, _w,                        \
             args_p->filter_w * args_p->filter_h * args_p->filter_c, 1,        \
             args_p->filter_c * args_p->filter_w, args_p->filter_c)
#define ELEM_OUT(_n, _c, _h, _w)                                               \
  tensor4dEl((int8_t *)args_p->y, _n, _c, _h, _w, args_p->out_nStride,         \
             args_p->out_cStride, args_p->out_hStride, args_p->out_wStride)
  // Calculate the scale of the values that come out of the matrix
  // multiplication part of the calculation.
  float matMulScale = args_p->inScale * args_p->filterScale;
  // Separate pass for every image
  for (int n = 0; n < args_p->in_n; n++) {
    // Every 3d filter (CHW dimensions) creates channel in output image
    for (int c = 0; c < args_p->filter_k; c++) {
      // For every element in 2d image
      for (int h = 0; h < args_p->out_h; h++) {
        for (int w = 0; w < args_p->out_w; w++) {
          int y = (h * args_p->u) - args_p->pad_h;
          int x = (w * args_p->v) - args_p->pad_w;

          // Apply 2d filter
          int32_t conv_sum = 0;
          for (int fh = 0; fh < args_p->filter_h; fh++) {
            for (int fw = 0; fw < args_p->filter_w; fw++) {
              int ih = y + fh * args_p->dilation_h;
              int iw = x + fw * args_p->dilation_w;

              if (ih >= 0 && iw >= 0 && ih < args_p->in_h &&
                  iw < args_p->in_w) {
                /**
                 * Each input channel is filtered by channel filter and added to
                 * resulting sum In the end all channels of input image folded
                 * into one
                 */
                for (int fc = 0; fc < args_p->filter_c; fc++) {
                  int32_t I = ELEM_IN(n, fc, ih, iw);
                  int32_t F = (args_p->filter_format == ETDNN_TENSOR_NCHW)
                                  ? ELEM_FILTER1(c, fc, fh, fw)
                                  : ELEM_FILTER2(c, fc, fh, fw);
                  conv_sum +=
                      (I - args_p->inOffset) * (F - args_p->filterOffset);
                }
              }
            }
          }

          if (args_p->b) {
            // Scale the bias to match the scale of the matrix multiplication.
            int32_t B = std::round(
                float(((int32_t *)args_p->b)[c] - args_p->biasOffset) *
                (args_p->biasScale / matMulScale));
            conv_sum += B;
          }

          int8_t &oelem = ELEM_OUT(n, c, h, w);
          // Scale the result back to the expected destination scale.
          oelem = quantization::clip<int32_t, int8_t>(
              std::round(float(conv_sum) * (matMulScale / args_p->outScale) +
                         args_p->outOffset));
        }
      }
    }
  }
#undef ELEM_OUT
#undef ELEM_FILTER2
#undef ELEM_FILTER1
#undef ELEM_IN
}

/**
 * y = α * (W(x) + b) + β * y
 */
void cpuConvolutionForward(ConvolutionForward_KernelArgs_t *args_p) {
  assert(args_p->filter_format == ETDNN_TENSOR_NCHW ||
         args_p->filter_format == ETDNN_TENSOR_NHWC);
  assert(args_p->in_c == args_p->filter_c);
  assert(args_p->in_n == args_p->out_n);
  assert(args_p->filter_k == args_p->out_c);
  assert(args_p->dilation_h > 0);
  assert(args_p->dilation_w > 0);

  switch (args_p->data_type) {
  case ETDNN_DATA_FLOAT:
    cpuConvolutionForwardFloat(args_p);
    break;
  case ETDNN_DATA_INT8Q:
    assert(args_p->alpha.f == 1.0f);
    assert(args_p->beta.f == 0.0f);
    cpuConvolutionForwardInt8Q(args_p);
    break;
  default:
    abort();
    break;
  }
}

void cpuBatchNormalizationForwardInferenceFloat(
    BatchNormalizationForwardInference_KernelArgs_t *args_p) {
#define ELEM_IN(_n, _c, _h, _w)                                                \
  tensor4dEl((const float *)args_p->x, _n, _c, _h, _w, args_p->in_nStride,     \
             args_p->in_cStride, args_p->in_hStride, args_p->in_wStride)
#define ELEM_SCALE(_n, _c, _h, _w)                                             \
  tensor4dEl((const float *)args_p->bnScale, _n, _c, _h, _w,                   \
             args_p->bn_nStride, args_p->bn_cStride, args_p->bn_hStride,       \
             args_p->bn_wStride)
#define ELEM_BIAS(_n, _c, _h, _w)                                              \
  tensor4dEl((const float *)args_p->bnBias, _n, _c, _h, _w,                    \
             args_p->bn_nStride, args_p->bn_cStride, args_p->bn_hStride,       \
             args_p->bn_wStride)
#define ELEM_MEAN(_n, _c, _h, _w)                                              \
  tensor4dEl((const float *)args_p->estimatedMean, _n, _c, _h, _w,             \
             args_p->bn_nStride, args_p->bn_cStride, args_p->bn_hStride,       \
             args_p->bn_wStride)
#define ELEM_VAR(_n, _c, _h, _w)                                               \
  tensor4dEl((const float *)args_p->estimatedVariance, _n, _c, _h, _w,         \
             args_p->bn_nStride, args_p->bn_cStride, args_p->bn_hStride,       \
             args_p->bn_wStride)
#define ELEM_OUT(_n, _c, _h, _w)                                               \
  tensor4dEl((float *)args_p->y, _n, _c, _h, _w, args_p->out_nStride,          \
             args_p->out_cStride, args_p->out_hStride, args_p->out_wStride)
  for (int n = 0; n < args_p->n; n++) {
    int bnn = 0;
    for (int c = 0; c < args_p->c; c++) {
      int bnc = c;
      for (int h = 0; h < args_p->h; h++) {
        int bnh = (args_p->mode == ETDNN_BATCHNORM_PER_ACTIVATION) ? h : 0;
        for (int w = 0; w < args_p->w; w++) {
          int bnw = (args_p->mode == ETDNN_BATCHNORM_PER_ACTIVATION) ? w : 0;
          float &oelem = ELEM_OUT(n, c, h, w);
          float in_elem = ELEM_IN(n, c, h, w);
          float scale = ELEM_SCALE(bnn, bnc, bnh, bnw);
          float bias = ELEM_BIAS(bnn, bnc, bnh, bnw);
          float mean = ELEM_MEAN(bnn, bnc, bnh, bnw);
          float var = ELEM_VAR(bnn, bnc, bnh, bnw);
          float result =
              (scale * (in_elem - mean) / sqrtf(args_p->epsilon + var) + bias);

          blendResult(args_p->alpha.f, result, args_p->beta.f, oelem);
        }
      }
    }
  }

  return;
#undef ELEM_OUT
#undef ELEM_VAR
#undef ELEM_MEAN
#undef ELEM_BIAS
#undef ELEM_SCALE
#undef ELEM_IN
}

/**
 * y = α * (bnScale * (x - estimatedMean) / sqrt(epsilon + estimatedVariance) +
 * bnBias) + β * y
 */
void cpuBatchNormalizationForwardInference(
    BatchNormalizationForwardInference_KernelArgs_t *args_p) {
  switch (args_p->data_type) {
  case ETDNN_DATA_FLOAT:
    cpuBatchNormalizationForwardInferenceFloat(args_p);
    break;
  default:
    abort();
    break;
  }
}

void cpuAddTensorFloat(AddTensor_KernelArgs_t *args_p) {
#define ELEM_IN(_n, _c, _h, _w)                                                \
  tensor4dEl((const float *)args_p->A, _n, _c, _h, _w, args_p->in_nStride,     \
             args_p->in_cStride, args_p->in_hStride, args_p->in_wStride)
#define ELEM_OUT(_n, _c, _h, _w)                                               \
  tensor4dEl((float *)args_p->C, _n, _c, _h, _w, args_p->out_nStride,          \
             args_p->out_cStride, args_p->out_hStride, args_p->out_wStride)
  for (int n = 0; n < args_p->out_n; n++) {
    int in = (args_p->in_n == args_p->out_n) ? n : 0;
    for (int c = 0; c < args_p->out_c; c++) {
      int ic = (args_p->in_c == args_p->out_c) ? c : 0;
      for (int h = 0; h < args_p->out_h; h++) {
        int ih = (args_p->in_h == args_p->out_h) ? h : 0;
        for (int w = 0; w < args_p->out_w; w++) {
          int iw = (args_p->in_w == args_p->out_w) ? w : 0;
          float &oelem = ELEM_OUT(n, c, h, w);
          float in_elem = ELEM_IN(in, ic, ih, iw);
          blendResult(args_p->alpha.f, in_elem, args_p->beta.f, oelem);
        }
      }
    }
  }

  return;
#undef ELEM_OUT
#undef ELEM_IN
}

/**
 * c = α * A + β * C
 */
void cpuAddTensor(AddTensor_KernelArgs_t *args_p) {
  switch (args_p->data_type) {
  case ETDNN_DATA_FLOAT:
    cpuAddTensorFloat(args_p);
    break;
  default:
    abort();
    break;
  }
}

/* Sigmoid activation function */
float sigmoid(float x) { return 1.0f / (1.0f + expf(-x)); }

/* Relu activation function */
float relu(float x) { return fmaxf(0.0f, x); }

/* Tanh activation function (float version instead of double in math.h) */
float tanhf(float x) { return 2.0f / (1.0f + expf(-2.0f * x)) - 1.0f; }

/* Activation Forward CPU implemetation */
template <typename F>
void cpuActivationForwardFloat(F func, ActivationForward_KernelArgs_t *args_p) {
#define ELEM_IN(_n, _c, _h, _w)                                                \
  tensor4dEl((const float *)args_p->x, _n, _c, _h, _w, args_p->in_nStride,     \
             args_p->in_cStride, args_p->in_hStride, args_p->in_wStride)
#define ELEM_OUT(_n, _c, _h, _w)                                               \
  tensor4dEl((float *)args_p->y, _n, _c, _h, _w, args_p->out_nStride,          \
             args_p->out_cStride, args_p->out_hStride, args_p->out_wStride)
  for (int n = 0; n < args_p->n; n++) {
    for (int c = 0; c < args_p->c; c++) {
      for (int h = 0; h < args_p->h; h++) {
        for (int w = 0; w < args_p->w; w++) {
          float &oelem = ELEM_OUT(n, c, h, w);
          float in_elem = ELEM_IN(n, c, h, w);
          float res = func(in_elem);
          blendResult(args_p->alpha.f, res, args_p->beta.f, oelem);
        }
      }
    }
  }

  return;
#undef ELEM_OUT
#undef ELEM_IN
}

/**
 * y = α * F(x) + β * y
 */
void cpuActivationForward(ActivationForward_KernelArgs_t *args_p) {
  /* Currently support only float32 */
  if (args_p->data_type != ETDNN_DATA_FLOAT) {
    abort();
    return;
  }

  switch (args_p->ac_function) {
  case ETDNN_ACTIVATION_SIGMOID:
    cpuActivationForwardFloat(sigmoid, args_p);
    break;
  case ETDNN_ACTIVATION_RELU:
    cpuActivationForwardFloat(relu, args_p);
    break;
  case ETDNN_ACTIVATION_TANH:
    cpuActivationForwardFloat(tanhf, args_p);
    break;
  default:
    abort();
    break;
  }
}

/* Pooling Forward CPU implemetation */
void cpuPoolingForwardFloat(PoolingForward_KernelArgs_t *args_p) {
#define ELEM_IN(_n, _c, _h, _w)                                                \
  tensor4dEl((const float *)args_p->x, _n, _c, _h, _w, args_p->in_nStride,     \
             args_p->in_cStride, args_p->in_hStride, args_p->in_wStride)
#define ELEM_OUT(_n, _c, _h, _w)                                               \
  tensor4dEl((float *)args_p->y, _n, _c, _h, _w, args_p->out_nStride,          \
             args_p->out_cStride, args_p->out_hStride, args_p->out_wStride)
  for (int n = 0; n < args_p->out_n; n++) {
    for (int c = 0; c < args_p->out_c; c++) {
      int y = -args_p->vertical_pad;
      for (int h = 0; h < args_p->out_h; h++, y += args_p->vertical_stride) {
        int x = -args_p->horizontal_pad;
        for (int w = 0; w < args_p->out_w;
             w++, x += args_p->horizontal_stride) {
          int start_x = std::max(x, 0);
          int start_y = std::max(y, 0);
          int limit_x = std::min(x + args_p->window_w, args_p->in_w);
          int limit_y = std::min(y + args_p->window_h, args_p->in_h);

          assert(limit_x > 0);
          assert(limit_y > 0);

          float max_val = ELEM_IN(n, c, start_y, start_x);
          float sum = 0.0f;

          for (int i = start_y; i < limit_y; i++) {
            for (int j = start_x; j < limit_x; j++) {
              float cur_val = ELEM_IN(n, c, i, j);
              max_val = fmaxf(max_val, cur_val);
              sum += cur_val;
            }
          }

          float result;
          switch (args_p->pooling_function) {
          case ETDNN_POOLING_MAX:
            result = max_val;
            break;
          case ETDNN_POOLING_AVERAGE_COUNT_INCLUDE_PADDING:
            result = sum / (args_p->window_w * args_p->window_h);
            break;
          case ETDNN_POOLING_AVERAGE_COUNT_EXCLUDE_PADDING:
            result = sum / ((limit_x - start_x) * (limit_y - start_y));
            break;
          default:
            abort();
          }
          float &out_elem = ELEM_OUT(n, c, h, w);
          blendResult(args_p->alpha.f, result, args_p->beta.f, out_elem);
        }
      }
    }
  }

  return;
#undef ELEM_OUT
#undef ELEM_IN
}

/* Pooling Forward CPU implemetation */
void cpuPoolingForwardInt8Q(PoolingForward_KernelArgs_t *args_p) {
#define ELEM_IN(_n, _c, _h, _w)                                                \
  tensor4dEl((const int8_t *)args_p->x, _n, _c, _h, _w, args_p->in_nStride,    \
             args_p->in_cStride, args_p->in_hStride, args_p->in_wStride)
#define ELEM_OUT(_n, _c, _h, _w)                                               \
  tensor4dEl((int8_t *)args_p->y, _n, _c, _h, _w, args_p->out_nStride,         \
             args_p->out_cStride, args_p->out_hStride, args_p->out_wStride)
  for (int n = 0; n < args_p->out_n; n++) {
    for (int c = 0; c < args_p->out_c; c++) {
      int y = -args_p->vertical_pad;
      for (int h = 0; h < args_p->out_h; h++, y += args_p->vertical_stride) {
        int x = -args_p->horizontal_pad;
        for (int w = 0; w < args_p->out_w;
             w++, x += args_p->horizontal_stride) {
          int start_x = std::max(x, 0);
          int start_y = std::max(y, 0);
          int limit_x = std::min(x + args_p->window_w, args_p->in_w);
          int limit_y = std::min(y + args_p->window_h, args_p->in_h);

          assert(limit_x > 0);
          assert(limit_y > 0);

          int32_t max_val = ELEM_IN(n, c, start_y, start_x);
          int32_t sum = 0;

          for (int i = start_y; i < limit_y; i++) {
            for (int j = start_x; j < limit_x; j++) {
              int32_t cur_val = ELEM_IN(n, c, i, j);
              max_val = std::max(max_val, cur_val);
              sum += cur_val - args_p->inOffset;
            }
          }

          int32_t int_val;
          float invScale;
          switch (args_p->pooling_function) {
          case ETDNN_POOLING_MAX:
            int_val = max_val - args_p->inOffset;
            invScale = args_p->inScale / args_p->outScale;
            break;
          case ETDNN_POOLING_AVERAGE_COUNT_INCLUDE_PADDING:
            int_val = sum;
            invScale = args_p->inScale / args_p->outScale /
                       (args_p->window_w * args_p->window_h);
            break;
          case ETDNN_POOLING_AVERAGE_COUNT_EXCLUDE_PADDING:
            int_val = sum;
            invScale = args_p->inScale / args_p->outScale /
                       ((limit_x - start_x) * (limit_y - start_y));
            break;
          default:
            abort();
          }
          int8_t &out_elem = ELEM_OUT(n, c, h, w);
          out_elem = quantization::clip<int32_t, int8_t>(
              std::round(float(int_val) * invScale + args_p->outOffset));
        }
      }
    }
  }

  return;
#undef ELEM_OUT
#undef ELEM_IN
}

/**
 * y = α * F(x) + β * y
 */
void cpuPoolingForward(PoolingForward_KernelArgs_t *args_p) {
  assert(args_p->out_n == args_p->in_n);
  assert(args_p->out_c == args_p->in_c);

  switch (args_p->data_type) {
  case ETDNN_DATA_FLOAT:
    cpuPoolingForwardFloat(args_p);
    break;
  case ETDNN_DATA_INT8Q:
    assert(args_p->alpha.f == 1.0f);
    assert(args_p->beta.f == 0.0f);
    cpuPoolingForwardInt8Q(args_p);
    break;
  default:
    abort();
    break;
  }
}

void cpuSoftmaxForwardFloat(SoftmaxForward_KernelArgs_t *args_p) {
#define ELEM_IN(_n, _c, _h, _w)                                                \
  tensor4dEl((const float *)args_p->x, _n, _c, _h, _w, args_p->in_nStride,     \
             args_p->in_cStride, args_p->in_hStride, args_p->in_wStride)
#define ELEM_OUT(_n, _c, _h, _w)                                               \
  tensor4dEl((float *)args_p->y, _n, _c, _h, _w, args_p->out_nStride,          \
             args_p->out_cStride, args_p->out_hStride, args_p->out_wStride)
  for (int n = 0; n < args_p->n; n++) {
    float sum = 0.0;

    for (int c = 0; c < args_p->c; c++) {
      for (int h = 0; h < args_p->h; h++) {
        for (int w = 0; w < args_p->w; w++) {
          sum += expf(ELEM_IN(n, c, h, w));
        }
      }
    }

    for (int c = 0; c < args_p->c; c++) {
      for (int h = 0; h < args_p->h; h++) {
        for (int w = 0; w < args_p->w; w++) {
          float elem_in = ELEM_IN(n, c, h, w);
          float &elem_out = ELEM_OUT(n, c, h, w);

          float local_result = expf(elem_in) / sum;
          blendResult(args_p->alpha.f, local_result, args_p->beta.f, elem_out);
        }
      }
    }
  }
#undef ELEM_OUT
#undef ELEM_IN
}

/**
 * Y = α * softmax(X) + β * Y
 */
void cpuSoftmaxForward(SoftmaxForward_KernelArgs_t *args_p) {
  assert(args_p->mode == ETDNN_SOFTMAX_MODE_INSTANCE);

  switch (args_p->data_type) {
  case ETDNN_DATA_FLOAT:
    cpuSoftmaxForwardFloat(args_p);
    break;
  default:
    abort();
    break;
  }
}

void cpuTransformTensorFloat(TransformTensor_KernelArgs_t *args_p) {
#define ELEM_IN(_n, _c, _h, _w)                                                \
  tensor4dEl((const float *)args_p->x, _n, _c, _h, _w, args_p->in_nStride,     \
             args_p->in_cStride, args_p->in_hStride, args_p->in_wStride)
#define ELEM_OUT(_n, _c, _h, _w)                                               \
  tensor4dEl((float *)args_p->y, _n, _c, _h, _w, args_p->out_nStride,          \
             args_p->out_cStride, args_p->out_hStride, args_p->out_wStride)
  for (int n = 0; n < args_p->n; n++) {
    for (int c = 0; c < args_p->c; c++) {
      for (int h = 0; h < args_p->h; h++) {
        for (int w = 0; w < args_p->w; w++) {
          float elem_in = ELEM_IN(n, c, h, w);
          float &elem_out = ELEM_OUT(n, c, h, w);
          blendResult(args_p->alpha.f, elem_in, args_p->beta.f, elem_out);
        }
      }
    }
  }
#undef ELEM_OUT
#undef ELEM_IN
}

void cpuTransformTensorInt8(TransformTensor_KernelArgs_t *args_p) {
#define ELEM_IN(_n, _c, _h, _w)                                                \
  tensor4dEl((const int8_t *)args_p->x, _n, _c, _h, _w, args_p->in_nStride,    \
             args_p->in_cStride, args_p->in_hStride, args_p->in_wStride)
#define ELEM_OUT(_n, _c, _h, _w)                                               \
  tensor4dEl((int8_t *)args_p->y, _n, _c, _h, _w, args_p->out_nStride,         \
             args_p->out_cStride, args_p->out_hStride, args_p->out_wStride)
  for (int n = 0; n < args_p->n; n++) {
    for (int c = 0; c < args_p->c; c++) {
      for (int h = 0; h < args_p->h; h++) {
        for (int w = 0; w < args_p->w; w++) {
          ELEM_OUT(n, c, h, w) = ELEM_IN(n, c, h, w);
        }
      }
    }
  }
#undef ELEM_OUT
#undef ELEM_IN
}

void cpuTransformTensor(TransformTensor_KernelArgs_t *args_p) {
  switch (args_p->data_type) {
  case ETDNN_DATA_FLOAT:
    cpuTransformTensorFloat(args_p);
    break;
  case ETDNN_DATA_INT8:
    assert(args_p->alpha.f == 1.0f);
    assert(args_p->beta.f == 0.0f);
    cpuTransformTensorInt8(args_p);
    break;
  default:
    abort();
    break;
  }
}

template <typename T> void cpuInsertImpl(Insert_KernelArgs_t *args_p) {
#define ELEM_IN(_d0, _d1, _d2, _d3)                                            \
  tensor4dEl((const T *)args_p->x, _d0, _d1, _d2, _d3, args_p->x_strides[0],   \
             args_p->x_strides[1], args_p->x_strides[2], args_p->x_strides[3])
#define ELEM_OUT(_d0, _d1, _d2, _d3)                                           \
  tensor4dEl((T *)args_p->y, _d0, _d1, _d2, _d3, args_p->y_strides[0],         \
             args_p->y_strides[1], args_p->y_strides[2], args_p->y_strides[3])
  assert(args_p->dimensions == 4);

  for (int d0 = 0; d0 < args_p->sizes[0]; d0++) {
    for (int d1 = 0; d1 < args_p->sizes[1]; d1++) {
      for (int d2 = 0; d2 < args_p->sizes[2]; d2++) {
        for (int d3 = 0; d3 < args_p->sizes[3]; d3++) {
          ELEM_OUT(d0 + args_p->y_offsets[0], d1 + args_p->y_offsets[1],
                   d2 + args_p->y_offsets[2], d3 + args_p->y_offsets[3]) =
              ELEM_IN(d0 + args_p->x_offsets[0], d1 + args_p->x_offsets[1],
                      d2 + args_p->x_offsets[2], d3 + args_p->x_offsets[3]);
        }
      }
    }
  }
#undef ELEM_IN
#undef ELEM_OUT
}

void cpuInsert(Insert_KernelArgs_t *args_p) {
  switch (args_p->data_type) {
  case ETDNN_DATA_FLOAT:
    cpuInsertImpl<float>(args_p);
    break;
  case ETDNN_DATA_INT64:
    cpuInsertImpl<int64_t>(args_p);
    break;
  default:
    abort();
    break;
  }
}

void cpuBatchedAdd(BatchedAdd_KernelArgs_t *args_p) {
  int i = 0;

  for (int b = 0; b < args_p->n; b++) {
    for (int s = 0; s < args_p->slice_size; s++, i++) {
      args_p->dest[i] = args_p->batch[i] + args_p->slice[s];
    }
  }
}

void cpuQuantizedBatchedAdd(QuantizedBatchedAdd_KernelArgs_t *args_p) {
  int i = 0;

  for (int b = 0; b < args_p->n; b++) {
    for (int s = 0; s < args_p->slice_size; s++, i++) {
      float batchVal = quantization::dequantize(
          args_p->batch[i], args_p->batchScale, args_p->batchOffset);
      float sliceVal = quantization::dequantize(
          args_p->slice[s], args_p->sliceScale, args_p->sliceOffset);
      float destVal = batchVal + sliceVal;
      args_p->dest[i] = quantization::quantize(destVal, args_p->destScale,
                                               args_p->destOffset);
    }
  }
}

void cpuLRNFloat(LRN_KernelArgs_t *args_p) {
#define ELEM_IN(_n, _c, _h, _w)                                                \
  tensor4dEl((const float *)args_p->x, _n, _c, _h, _w, args_p->in_nStride,     \
             args_p->in_cStride, args_p->in_hStride, args_p->in_wStride)
#define ELEM_OUT(_n, _c, _h, _w)                                               \
  tensor4dEl((float *)args_p->y, _n, _c, _h, _w, args_p->out_nStride,          \
             args_p->out_cStride, args_p->out_hStride, args_p->out_wStride)
#define ELEM_SCALE(_n, _c, _h, _w)                                             \
  tensor4dEl((float *)args_p->scale, _n, _c, _h, _w, args_p->scale_nStride,    \
             args_p->scale_cStride, args_p->scale_hStride,                     \
             args_p->scale_wStride)
  float window_size_half = args_p->halfWindowSize;
  float window_size = window_size_half * 2 + 1;
  float normed_alpha = args_p->alpha.f / window_size;

  for (int n = 0; n < args_p->n; n++) {
    for (int c = 0; c < args_p->c; c++) {
      int c_min = (c >= window_size_half) ? (c - window_size_half) : 0;
      int c_max = c + window_size_half;
      int c_limit = args_p->c - 1;

      c_max = c_max < c_limit ? c_max : c_limit;

      for (int h = 0; h < args_p->h; h++) {
        for (int w = 0; w < args_p->w; w++) {
          float square_sum = 0.0;
          float scale;
          float norm_factor;

          for (int c_win = c_min; c_win <= c_max; c_win++) {
            float in_value = ELEM_IN(n, c_win, h, w);

            square_sum += in_value * in_value;
          }

          scale = normed_alpha * square_sum + args_p->k.f;
          norm_factor = powf(scale, -args_p->beta.f);

          ELEM_SCALE(n, c, h, w) = scale;
          ELEM_OUT(n, c, h, w) = ELEM_IN(n, c, h, w) * norm_factor;
        }
      }
    }
  }
#undef ELEM_IN
#undef ELEM_OUT
#undef ELEM_SCALE
}

void cpuLRN(LRN_KernelArgs_t *args_p) {
  switch (args_p->data_type) {
  case ETDNN_DATA_FLOAT:
    cpuLRNFloat(args_p);
    break;
  default:
    abort();
    break;
  }
}

void cpuGather(Gather_KernelArgs_t *args_p) {
  size_t slice_size = args_p->data_ydim * args_p->elem_size;
  const char *data_p = (const char *)args_p->data;
  char *dest_p = (char *)args_p->dest;

  for (int i = 0; i < args_p->indices_n; i++) {
    memcpy(dest_p + slice_size * i, data_p + args_p->indices[i] * slice_size,
           slice_size);
  }
}

typedef struct {
  float f;
  size_t i;
} TopK_elem_t;

int compare_fn(const void *a, const void *b) {
  const TopK_elem_t *af = (const TopK_elem_t *)a;
  const TopK_elem_t *bf = (const TopK_elem_t *)b;

  return af->f < bf->f;
}

void cpuTopK(TopK_KernelArgs_t *args_p) {
  float *in = (float *)args_p->in;
  int in_sz = args_p->in_sz;
  int k = args_p->k;
  float *out = (float *)args_p->out;
  size_t *indices_out = args_p->indices_out;
  TopK_elem_t tmp_in[in_sz];
  for (int i = 0; i < in_sz; i++) {
    tmp_in[i].f = in[i];
    tmp_in[i].i = i;
  }

  qsort(tmp_in, in_sz, sizeof(tmp_in[0]), compare_fn);

  for (int i = 0; i < k; i++) {
    out[i] = tmp_in[i].f;
    indices_out[i] = tmp_in[i].i;
  }
}

void cpuSplat(Splat_KernelArgs_t *args_p) {
  if (args_p->dt == ETDNN_DATA_FLOAT) {
    float *data = (float *)args_p->y;
    for (size_t i = 0; i < args_p->count; i++) {
      data[i] = args_p->data.f;
    }
  } else if (args_p->dt == ETDNN_DATA_INT8) {
    int8_t *data = (int8_t *)args_p->y;
    for (size_t i = 0; i < args_p->count; i++) {
      data[i] = args_p->data.i8;
    }
  } else if (args_p->dt == ETDNN_DATA_INT32) {
    int32_t *data = (int32_t *)args_p->y;
    for (size_t i = 0; i < args_p->count; i++) {
      data[i] = args_p->data.i32;
    }
  } else if (args_p->dt == ETDNN_DATA_INT64) {
    int64_t *data = (int64_t *)args_p->y;
    for (size_t i = 0; i < args_p->count; i++) {
      data[i] = args_p->data.i64;
    }
  }
}

void cpuQuantizedArithOp(QuantizedArithOp_KernelArgs_t *args_p) {
  for (int i = 0; i < args_p->n; i++) {
    float aVal =
        quantization::dequantize(args_p->a[i], args_p->aScale, args_p->aOffset);
    float bVal =
        quantization::dequantize(args_p->b[i], args_p->bScale, args_p->bOffset);
    float yVal;
    switch (args_p->op) {
    case ETDNN_TENSOR_ARITH_OP_ADD:
      yVal = aVal + bVal;
      break;
    case ETDNN_TENSOR_ARITH_OP_SUB:
      yVal = aVal - bVal;
      break;
    case ETDNN_TENSOR_ARITH_OP_MUL:
      yVal = aVal * bVal;
      break;
    case ETDNN_TENSOR_ARITH_OP_DIV:
      yVal = aVal / bVal;
      break;
    case ETDNN_TENSOR_ARITH_OP_MAX:
      yVal = std::max(aVal, bVal);
      break;
    case ETDNN_TENSOR_ARITH_OP_MIN:
      yVal = std::min(aVal, bVal);
      break;
    default:
      abort();
    }
    args_p->y[i] =
        quantization::quantize(yVal, args_p->yScale, args_p->yOffset);
  }
}
