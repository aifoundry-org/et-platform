#ifndef ETDNN_BIN_H
#define ETDNN_BIN_H

/****** /usr/include/cudnn.h ******/

/* Maximum supported number of tensor dimensions */
#define ETDNN_DIM_MAX 8

/*
 * CUDNN return codes
 */
typedef enum {
  ETDNN_STATUS_SUCCESS = 0,
  ETDNN_STATUS_NOT_INITIALIZED = 1,
  ETDNN_STATUS_ALLOC_FAILED = 2,
  ETDNN_STATUS_BAD_PARAM = 3,
  ETDNN_STATUS_INTERNAL_ERROR = 4,
  ETDNN_STATUS_INVALID_VALUE = 5,
  ETDNN_STATUS_ARCH_MISMATCH = 6,
  ETDNN_STATUS_MAPPING_ERROR = 7,
  ETDNN_STATUS_EXECUTION_FAILED = 8,
  ETDNN_STATUS_NOT_SUPPORTED = 9,
  ETDNN_STATUS_LICENSE_ERROR = 10,
  ETDNN_STATUS_RUNTIME_PREREQUISITE_MISSING = 11,
  ETDNN_STATUS_RUNTIME_IN_PROGRESS = 12,
  ETDNN_STATUS_RUNTIME_FP_OVERFLOW = 13,
} etdnnStatus_t;

/*
 * CUDNN data type
 */
typedef enum {
  ETDNN_DATA_FLOAT = 0,
  ETDNN_DATA_DOUBLE = 1,
  ETDNN_DATA_HALF = 2,
  ETDNN_DATA_INT8 = 3,
  ETDNN_DATA_INT32 = 4,
  ETDNN_DATA_INT8x4 = 5,
  ETDNN_DATA_UINT8 = 6,
  ETDNN_DATA_UINT8x4 = 7,
  ETDNN_DATA_INT64 = 18034, /* Esperanto extension! */
  ETDNN_DATA_INT8Q = 18035, /* Esperanto extension! */
} etdnnDataType_t;

/*
 * Mode for batch operations
 */
typedef enum {
  /* bnScale, bnBias tensor dims are 1xCxHxWx.. (one value per CHW...-slice,
     normalized over N slice) */
  ETDNN_BATCHNORM_PER_ACTIVATION = 0,

  /* bnScale, bnBias tensor dims are 1xCx1x1 (one value per C-dim normalized
     over Nx1xHxW subtensors) */
  ETDNN_BATCHNORM_SPATIAL = 1,

  /*
   * bnScale, bnBias tensor dims are 1xCx1x1 (one value per C-dim normalized
   * over Nx1xHxW subtensors). May be faster than CUDNN_BATCHNORM_SPATIAL but
   * imposes some limits on the range of values
   */
  ETDNN_BATCHNORM_SPATIAL_PERSISTENT = 2,
} etdnnBatchNormMode_t;

#define ETDNN_BN_MIN_EPSILON 1e-5

/*
 * convolution mode
 */
typedef enum {
  ETDNN_CONVOLUTION = 0,
  ETDNN_CROSS_CORRELATION = 1
} etdnnConvolutionMode_t;

/*
 * activation mode
 */
typedef enum {
  ETDNN_ACTIVATION_SIGMOID = 0,      // sigmoid function
  ETDNN_ACTIVATION_RELU = 1,         // rectified linear function
  ETDNN_ACTIVATION_TANH = 2,         // hyperbolic tangent function
  ETDNN_ACTIVATION_CLIPPED_RELU = 3, // clipped rectified linear function
  ETDNN_ACTIVATION_ELU = 4,          // exponential linear function
  ETDNN_ACTIVATION_IDENTITY =
      5 // selects the identity function, intended for bypassing the activation
        // step in cudnnConvolutionBiasActivationForward() (need to use
        // CUDNN_CONVOLUTION_FWD_ALGO_IMPLICIT_GEMM). Does not work with
        // cudnnActivationForward() or cudnnActivationBackward()
} etdnnActivationMode_t;

/*
 * pooling mode
 */
typedef enum {
  ETDNN_POOLING_MAX = 0,
  ETDNN_POOLING_AVERAGE_COUNT_INCLUDE_PADDING =
      1, /* count for average includes padded values */
  ETDNN_POOLING_AVERAGE_COUNT_EXCLUDE_PADDING =
      2, /* count for average does not include padded values */
  ETDNN_POOLING_MAX_DETERMINISTIC = 3
} etdnnPoolingMode_t;

/*
 * CUDNN math type
 * indicate if the use of Tensor Core Operations is permitted a given library
 * routine
 */
typedef enum {
  ETDNN_DEFAULT_MATH = 0,   // Tensor Core Operations are not used
  ETDNN_TENSOR_OP_MATH = 1, // use of Tensor Core Operations is permitted
} etdnnMathType_t;

/*
 * CUDNN propagate Nan
 */
typedef enum {
  ETDNN_NOT_PROPAGATE_NAN = 0,
  ETDNN_PROPAGATE_NAN = 1,
} etdnnNanPropagation_t;

/*
 * CUDNN Determinism
 */
typedef enum {
  ETDNN_NON_DETERMINISTIC = 0, // Results are not guaranteed to be reproducible
  ETDNN_DETERMINISTIC = 1,     // Results are guaranteed to be reproducible
} etdnnDeterminism_t;

typedef enum {
  ETDNN_TENSOR_NCHW =
      0, /* row major (wStride = 1, hStride = w). This tensor format specifies
            that the data is laid out in the following order: batch size,
            feature maps, rows, columns. The strides are implicitly defined in
            such a way that the data are contiguous in memory with no padding
            between images, feature maps, rows, and columns; the columns are the
            inner dimension and the images are the outermost dimension. */
  ETDNN_TENSOR_NHWC =
      1, /* feature maps interleaved ( cStride = 1 ) This tensor format
            specifies that the data is laid out in the following order: batch
            size, rows, columns, feature maps. The strides are implicitly
            defined in such a way that the data are contiguous in memory with no
            padding between images, rows, columns, and feature maps; the feature
            maps are the inner dimension and the images are the outermost
            dimension.*/
  ETDNN_TENSOR_NCHW_VECT_C =
      2 /* each image point is vector of element of C : the length of the vector
           is carried by the data type. This tensor format specifies that the
           data is laid out in the following order: batch size, feature maps,
           rows, columns. However, each element of the tensor is a vector of
           multiple feature maps. The length of the vector is carried by the
           data type of the tensor. The strides are implicitly defined in such a
           way that the data are contiguous in memory with no padding between
           images, feature maps, rows, and columns; the columns are the inner
           dimension and the images are the outermost dimension. This format is
           only supported with tensor data types CUDNN_DATA_INT8x4 and
           CUDNN_DATA_UINT8x4. */
} etdnnTensorFormat_t;

/**
 * helps the choice of the algorithm used for the forward convolution
 */
typedef enum {
  ETDNN_CONVOLUTION_FWD_NO_WORKSPACE =
      0, // guaranteed to return an algorithm that does not require any extra
         // workspace to be provided by the user
  ETDNN_CONVOLUTION_FWD_PREFER_FASTEST =
      1, // return the fastest algorithm regardless how much workspace is needed
         // to execute it
  ETDNN_CONVOLUTION_FWD_SPECIFY_WORKSPACE_LIMIT =
      2, // return the fastest algorithm that fits within the memory limit that
         // the user provided
} etdnnConvolutionFwdPreference_t;

/**
 * different algorithms available to execute the forward convolution operation.
 */
typedef enum {
  ETDNN_CONVOLUTION_FWD_ALGO_IMPLICIT_GEMM =
      0, // This algorithm expresses the convolution as a matrix product without
         // actually explicitly form the matrix that holds the input tensor
         // data.
  ETDNN_CONVOLUTION_FWD_ALGO_IMPLICIT_PRECOMP_GEMM =
      1, // This algorithm expresses the convolution as a matrix product without
         // actually explicitly form the matrix that holds the input tensor
         // data, but still needs some memory workspace to precompute some
         // indices in order to facilitate the implicit construction of the
         // matrix that holds the input tensor data
  ETDNN_CONVOLUTION_FWD_ALGO_GEMM =
      2, // This algorithm expresses the convolution as an explicit matrix
         // product. A significant memory workspace is needed to store the
         // matrix that holds the input tensor data.
  ETDNN_CONVOLUTION_FWD_ALGO_DIRECT =
      3, // This algorithm expresses the convolution as a direct convolution
         // (e.g without implicitly or explicitly doing a matrix
         // multiplication).
  ETDNN_CONVOLUTION_FWD_ALGO_FFT =
      4, // This algorithm uses the Fast-Fourier Transform approach to compute
         // the convolution. A significant memory workspace is needed to store
         // intermediate results.
  ETDNN_CONVOLUTION_FWD_ALGO_FFT_TILING =
      5, // This algorithm uses the Fast-Fourier Transform approach but splits
         // the inputs into tiles. A significant memory workspace is needed to
         // store intermediate results but less than
         // CUDNN_CONVOLUTION_FWD_ALGO_FFT for large size images.
  ETDNN_CONVOLUTION_FWD_ALGO_WINOGRAD =
      6, // This algorithm uses the Winograd Transform approach to compute the
         // convolution. A reasonably sized workspace is needed to store
         // intermediate results.
  ETDNN_CONVOLUTION_FWD_ALGO_WINOGRAD_NONFUSED =
      7, // This algorithm uses the Winograd Transform approach to compute the
         // convolution. Significant workspace may be needed to store
         // intermediate results.
  ETDNN_CONVOLUTION_FWD_ALGO_COUNT = 8
} etdnnConvolutionFwdAlgo_t;

typedef struct {
  etdnnConvolutionFwdAlgo_t
      algo; // The algorithm run to obtain the associated performance metrics.
  etdnnStatus_t
      status; // If any error occurs during the workspace allocation or timing
              // of cudnnConvolutionForward(), this status will represent that
              // error. Otherwise, this status will be the return status of
              // cudnnConvolutionForward().
  float time; // The execution time of cudnnConvolutionForward() (in
              // milliseconds).
  size_t memory;                  // The workspace size (in bytes).
  etdnnDeterminism_t determinism; // The determinism of the algorithm.
  etdnnMathType_t mathType;       // The math type provided to the algorithm.
  int reserved[3];                // Reserved space for future properties.
} etdnnConvolutionFwdAlgoPerf_t;

/*
 *  softmax algorithm
 */
typedef enum {
  ETDNN_SOFTMAX_FAST = 0, /* This implementation applies the straightforward
                             softmax operation. */
  ETDNN_SOFTMAX_ACCURATE =
      1, /* This implementation scales each point of the softmax input domain by
            its maximum value to avoid potential floating point overflows in the
            softmax evaluation. */
  ETDNN_SOFTMAX_LOG = 2 /* This entry performs the Log softmax operation,
                           avoiding overflows by scaling each point in the input
                           domain as in CUDNN_SOFTMAX_ACCURATE */
} etdnnSoftmaxAlgorithm_t;

typedef enum {
  ETDNN_SOFTMAX_MODE_INSTANCE =
      0, /* compute the softmax over all C, H, W for each N */
  ETDNN_SOFTMAX_MODE_CHANNEL =
      1 /* compute the softmax over all C for each H, W, N */
} etdnnSoftmaxMode_t;

typedef struct etdnnTensorStruct *etdnnTensorDescriptor_t;
typedef struct etdnnConvolutionStruct *etdnnConvolutionDescriptor_t;
typedef struct etdnnPoolingStruct *etdnnPoolingDescriptor_t;
typedef struct etdnnFilterStruct *etdnnFilterDescriptor_t;
typedef struct etdnnActivationStruct *etdnnActivationDescriptor_t;

struct etdnnContext;
typedef struct etdnnContext *etdnnHandle_t;

/****** ******/

#endif // ETDNN_BIN_H
