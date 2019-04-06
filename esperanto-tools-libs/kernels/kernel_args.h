#ifndef ETTEE_KERNEL_ARGS_H
#define ETTEE_KERNEL_ARGS_H

#include "../include/etblas.h"
#include "../include/etdnn.h"

typedef struct {
  etblasOperation_t transa;
  etblasOperation_t transb;
  int m;
  int n;
  int k;
  float alpha_val;
  float beta_val;
  int lda;
  int ldb;
  int ldc;
  const float *A;
  const float *B;
  float *C;
} Sgemm_v2_KernelArgs_t;

typedef struct {
  int m;
  int n;
  int k;
  const float *A;
  int lda;
  bool is_transa;
  const float *B;
  int ldb;
  bool is_transb;
  float *C;
  int ldc;
} MatMulTensor_KernelArgs_t;

typedef struct {
  int m;
  int n;
  int k;
  const int8_t *A;
  int lda;
  bool is_transa;
  const int8_t *B;
  int ldb;
  bool is_transb;
  int32_t *C;
  int ldc;
} MatMulTensorInt8To32_KernelArgs_t;

typedef struct {
  int m;
  int n;
  int k;
  const int8_t *A;
  int lda;
  bool is_transa;
  int32_t offseta;
  const int8_t *B;
  int ldb;
  bool is_transb;
  int32_t offsetb;
  int8_t *C;
  int ldc;
  int32_t offsetc;
  float invScale;
} QuantizedMatMul_KernelArgs_t;

typedef struct {
  // Filter h & w
  int filter_h;
  int filter_w;
  // Input tensor 4d dimensions
  int in_c;
  int in_h;
  int in_w;
  // Output tensor 4d dimensions
  int out_c;
  int out_h;
  int out_w;
  // batch size = in_n = out_n
  int batch_size;
  // Padding (zero-filled) for input tensor
  int pad_h;
  int pad_w;
  // Strides for filter applying at input tensor
  int u;
  int v;
  // Ditance between elements in filter applying at input tensor
  int dilation_h;
  int dilation_w;
  void *workspace; // Workspace = intermediate matrix after MatMul
  const void *b;   // Bias
  void *y;         // Output tensor data
  // Quantization parameters
  float inScale;
  int32_t inOffset;
  float filterScale;
  int32_t filterOffset;
  float biasScale;
  int32_t biasOffset;
  float outScale;
  int32_t outOffset;
} ConvTailNHWC_KernelArgs_t;

typedef union {
  float f;
  double d;
  unsigned short h;
  signed char i8;
  unsigned char u8;
  int i32;
  int64_t i64;
  signed char i8x4[4];
  unsigned char u8x4[4];
} DataType_Value_t;

typedef struct {
  etdnnDataType_t data_type;         // Type of in/out/filter elements
  etdnnTensorFormat_t filter_format; // Packing format for filter

  // Filter 4d dimensions
  int filter_k;
  int filter_c;
  int filter_h;
  int filter_w;
  // Input tensor 4d dimensions and strides for each dimension
  int in_n;
  int in_c;
  int in_h;
  int in_w;
  int in_nStride;
  int in_cStride;
  int in_hStride;
  int in_wStride;
  // Output tensor 4d dimensions and strides for each dimension
  int out_n;
  int out_c;
  int out_h;
  int out_w;
  int out_nStride;
  int out_cStride;
  int out_hStride;
  int out_wStride;
  // Padding (zero-filled) for input tensor
  int pad_h;
  int pad_w;
  // Strides for filter applying at input tensor
  int u;
  int v;
  // Ditance between elements in filter applying at input tensor
  int dilation_h;
  int dilation_w;
  // Algorithm worspace info
  void *workspace;
  size_t workSpaceSizeInBytes;
  const void *x; // Input tensor data
  const void *w; // Filter data
  const void *b; // Bias
  void *y;       // Output tensor data
  // Scaling values
  DataType_Value_t alpha;
  DataType_Value_t beta;
  // Quantization parameters
  float inScale;
  int32_t inOffset;
  float filterScale;
  int32_t filterOffset;
  float biasScale;
  int32_t biasOffset;
  float outScale;
  int32_t outOffset;
} ConvolutionForward_KernelArgs_t;

typedef struct {
  etdnnDataType_t data_type;    // Type of in/out elements
  etdnnDataType_t bn_data_type; // Type of bn elements
  int dimensions;               // Number of dimensions, must be 4 or 5
  etdnnBatchNormMode_t mode;    // Batch normalization mode

  int n;
  int c;
  int h;
  int w;

  int in_nStride;
  int in_cStride;
  int in_hStride;
  int in_wStride;

  int out_nStride;
  int out_cStride;
  int out_hStride;
  int out_wStride;

  int bn_nStride;
  int bn_cStride;
  int bn_hStride;
  int bn_wStride;

  const void *x; // Input tensor data
  void *y;       // Output tensot data

  // BN values
  const void *bnScale;
  const void *bnBias;
  const void *estimatedMean;
  const void *estimatedVariance;

  DataType_Value_t alpha;
  DataType_Value_t beta;
  float epsilon;
} BatchNormalizationForwardInference_KernelArgs_t;

typedef struct {
  etdnnDataType_t data_type; // Type of in/out elements
  int dimensions;            // Number of dimensions, should be <= 5

  int in_n;
  int in_c;
  int in_h;
  int in_w;
  int in_5; // Currently assume that no 5th dimension

  int out_n;
  int out_c;
  int out_h;
  int out_w;
  int out_5; // Currently assume that no 5th dimension

  int in_nStride;
  int in_cStride;
  int in_hStride;
  int in_wStride;
  int in_5Stride;

  int out_nStride;
  int out_cStride;
  int out_hStride;
  int out_wStride;
  int out_5Stride;

  const void *A; // Input tensor
  void *C;       // Output tensor
  DataType_Value_t alpha;
  DataType_Value_t beta;
} AddTensor_KernelArgs_t;

typedef struct {
  etdnnActivationMode_t ac_function;   // Activation function to apply
  etdnnNanPropagation_t nan_prop_mode; // Nan's propogation mode
  float coef;                // Special value for some activation functions
  etdnnDataType_t data_type; // Type of in/out elements
  int dimensions;            // Number of dimensions, should be <= 5

  int n;
  int c;
  int h;
  int w;

  int in_nStride;
  int in_cStride;
  int in_hStride;
  int in_wStride;

  int out_nStride;
  int out_cStride;
  int out_hStride;
  int out_wStride;

  const void *x; // Input tensor
  void *y;       // Output tensor
  DataType_Value_t alpha;
  DataType_Value_t beta;
} ActivationForward_KernelArgs_t;

typedef struct {
  etdnnPoolingMode_t pooling_function; // Pooling function to apply
  etdnnNanPropagation_t nan_prop_mode; // Nan's propogation mode

  int window_h;
  int window_w;
  int vertical_pad;
  int horizontal_pad;
  int vertical_stride;
  int horizontal_stride;

  etdnnDataType_t data_type; // Type of in/out elements
  int dimensions;            // Number of dimensions, should be <= 5

  int in_n;
  int in_c;
  int in_h;
  int in_w;

  int in_nStride;
  int in_cStride;
  int in_hStride;
  int in_wStride;

  int out_n;
  int out_c;
  int out_h;
  int out_w;

  int out_nStride;
  int out_cStride;
  int out_hStride;
  int out_wStride;

  const void *x; // Input tensor
  void *y;       // Output tensor
  DataType_Value_t alpha;
  DataType_Value_t beta;
  // Quantization parameters
  float inScale;
  int32_t inOffset;
  float outScale;
  int32_t outOffset;
} PoolingForward_KernelArgs_t;

typedef struct {
  etdnnDataType_t data_type;
  etdnnSoftmaxMode_t mode;
  int dimensions; // for now must be 4

  int n;
  int c;
  int h;
  int w;

  int in_nStride;
  int in_cStride;
  int in_hStride;
  int in_wStride;

  int out_nStride;
  int out_cStride;
  int out_hStride;
  int out_wStride;

  const void *x; // Input tensor
  void *y;       // Output tensor
  DataType_Value_t alpha;
  DataType_Value_t beta;
} SoftmaxForward_KernelArgs_t;

typedef struct {
  etdnnDataType_t data_type;
  int dimensions; // for now must be 4

  int n;
  int c;
  int h;
  int w;

  int in_nStride;
  int in_cStride;
  int in_hStride;
  int in_wStride;

  int out_nStride;
  int out_cStride;
  int out_hStride;
  int out_wStride;

  const void *x; // Input tensor
  void *y;       // Output tensor
  DataType_Value_t alpha;
  DataType_Value_t beta;
} TransformTensor_KernelArgs_t;

typedef struct {
  int n;
  int slice_size;

  const float *batch;
  const float *slice;
  float *dest;
} BatchedAdd_KernelArgs_t;

typedef struct {
  int n;
  int slice_size;

  const int8_t *batch;
  float batchScale;
  int32_t batchOffset;
  const int8_t *slice;
  float sliceScale;
  int32_t sliceOffset;
  int8_t *dest;
  float destScale;
  int32_t destOffset;
} QuantizedBatchedAdd_KernelArgs_t;

typedef struct {
  int elem_size;
  int64_t *indices;
  int indices_n;
  void *data;
  int data_xdim;
  int data_ydim;
  void *dest;
  int dest_xdim;
} Gather_KernelArgs_t;

typedef struct {
  etdnnDataType_t data_type;
  int dimensions; // for now must be 4

  int sizes[4];
  int x_offsets[4];
  int y_offsets[4];

  int x_strides[4];
  int y_strides[4];
  const void *x; // Input tensor
  void *y;       // Output tensor
} Insert_KernelArgs_t;

typedef struct {
  void *in;
  size_t in_sz;
  size_t *indices_out;
  int k;
  void *out;
} TopK_KernelArgs_t;

typedef struct {
  etdnnDataType_t data_type;
  int dimensions; // for now must be 4

  int n;
  int c;
  int h;
  int w;

  int in_nStride;
  int in_cStride;
  int in_hStride;
  int in_wStride;

  int out_nStride;
  int out_cStride;
  int out_hStride;
  int out_wStride;

  int scale_nStride;
  int scale_cStride;
  int scale_hStride;
  int scale_wStride;

  const void *x;
  void *y;
  void *scale;

  long halfWindowSize;
  DataType_Value_t k;
  DataType_Value_t alpha;
  DataType_Value_t beta;
} LRN_KernelArgs_t;

typedef struct {
  etdnnDataType_t dt;
  DataType_Value_t data;
  void *y;
  size_t count;
} Splat_KernelArgs_t;

typedef struct {
  etdnnArithOp_t op;
  int n;
  const int8_t *a;
  const int8_t *b;
  int8_t *y;
  // Quantization parameters
  float aScale;
  float bScale;
  float yScale;
  int32_t aOffset;
  int32_t bOffset;
  int32_t yOffset;
} QuantizedArithOp_KernelArgs_t;

#endif // ETTEE_KERNEL_ARGS_H
