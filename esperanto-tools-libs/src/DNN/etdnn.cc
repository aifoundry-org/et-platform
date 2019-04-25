#include "DNN/etdnn.h"
#include "../kernels/kernel_args.h"
#include "C-API/etrt.h"
#include "EsperantoRuntime.h"
#include "Support/HelperMacros.h"
#include "eti.h"
#include <assert.h>
#include <cstring>
#include <math.h>
#include <stdlib.h>

#define INCLUDE_FOR_HOST
#include "../kernels/sys_inc.h"
#undef INCLUDE_FOR_HOST

struct EtDnnContext {
  EtStream *etStream = nullptr;
};

static const int N_index = 0;
static const int C_index = 1;
static const int H_index = 2;
static const int W_index = 3;

/**
 * Internal cudnn structure hiding behing cudnnTensorDescriptor_t
 */
struct EtDnnTensorStruct {
  etdnnDataType_t dataType;
  int dimensions;

  int sizes[ETDNN_DIM_MAX];
  int strides[ETDNN_DIM_MAX];

  // Quantization parameters
  float scale = 0.0f;
  int32_t offset = 0;
};

/**
 * Internal cudnn structure hiding behing cudnnFilterDescriptor_t
 */
struct EtDnnFilterStruct {
  etdnnDataType_t dataType;
  etdnnTensorFormat_t format;
  int k;
  int c;
  int h;
  int w;

  // Quantization parameters
  float scale = 0.0f;
  int32_t offset = 0;
};

/**
 * Internal cudnn structure hiding behing cudnnConvolutionDescriptor_t
 */
struct EtDnnConvolutionStruct {
  int pad_h;
  int pad_w;
  int u;
  int v;
  int dilation_h;
  int dilation_w;
  etdnnConvolutionMode_t mode;
  etdnnDataType_t computeType;
  int groupCount;

  // Quantization parameters of bias
  float biasScale = 0.0f;
  int32_t biasOffset = 0;
};

/**
 * Internal cudnn structure hiding behing cudnnActivationDescriptor_t
 */
struct EtDnnActivationStruct {
  etdnnActivationMode_t mode;     /* activation function*/
  etdnnNanPropagation_t nan_prop; /* nan mode propagation*/
  double coef; /* clipping thresold for some special activation functions */
};

/**
 * Internal cudnn structure hiding behing cudnnPoolingDescriptor_t
 */
struct EtDnnPoolingStruct {
  etdnnPoolingMode_t mode;        /* pooling function*/
  etdnnNanPropagation_t nan_prop; /* nan mode propagation*/
  int window_h;                   /* window height */
  int window_w;                   /* window width */
  int vertical_pad;               /* vertical padding */
  int horizontal_pad;             /* horizontal padding */
  int vertical_stride;            /* vertical stride */
  int horizontal_stride;          /* horizontal stride */
};

#define ETDNN_VERSION 7102

template <typename T, class A>
void setArgsAlphaBeta(A *args, const void *alpha, const void *beta) {
  T alpha_val = *(const T *)alpha;
  T beta_val = *(const T *)beta;
  *(T *)&args->alpha = alpha_val;
  *(T *)&args->beta = beta_val;
}

template <class A>
std::string
setArgsAlphaBetaAndFixKernelName(A *args, const std::string &kernel_name,
                                 etdnnDataType_t data_type, const void *alpha,
                                 const void *beta) {
  switch (data_type) {
  case ETDNN_DATA_FLOAT:
    setArgsAlphaBeta<float>(args, alpha, beta);
    return kernel_name + "_Float";
  case ETDNN_DATA_INT8Q:
    setArgsAlphaBeta<float>(args, alpha, beta);
    return kernel_name + "_Int8Q";
  case ETDNN_DATA_INT8:
    setArgsAlphaBeta<float>(args, alpha, beta);
    return kernel_name + "_Int8";
  default:
    abort();
    return "";
  }
}

std::string fixKernelNameByDataType(const std::string &kernel_name,
                                    etdnnDataType_t data_type) {
  switch (data_type) {
  case ETDNN_DATA_FLOAT:
    return kernel_name + "_Float";
  case ETDNN_DATA_INT64:
    return kernel_name + "_Int64";
  case ETDNN_DATA_INT8Q:
    return kernel_name + "_Int8Q";
  case ETDNN_DATA_INT8:
    return kernel_name + "_Int8";
  default:
    abort();
    return "";
  }
}

/**
 * Check that tensor dense packed in memory in specific format (without holes in
 * memory).
 */
static bool isTensorInDenseFormat(EtDnnTensorStruct *tensorStruct,
                                  etdnnTensorFormat_t format) {
  int nStride, cStride, hStride, wStride;
  switch (format) {
  case ETDNN_TENSOR_NCHW:
    wStride = 1;
    hStride = tensorStruct->sizes[W_index];
    cStride = tensorStruct->sizes[W_index] * tensorStruct->sizes[H_index];
    nStride = tensorStruct->sizes[W_index] * tensorStruct->sizes[H_index] *
              tensorStruct->sizes[C_index];
    break;
  case ETDNN_TENSOR_NHWC:
    cStride = 1;
    wStride = tensorStruct->sizes[C_index];
    hStride = tensorStruct->sizes[C_index] * tensorStruct->sizes[W_index];
    nStride = tensorStruct->sizes[C_index] * tensorStruct->sizes[W_index] *
              tensorStruct->sizes[H_index];
    break;
  case ETDNN_TENSOR_NCHW_VECT_C:
  default:
    // not implemented yet
    abort();
  }

  return (nStride == tensorStruct->strides[N_index]) &&
         (cStride == tensorStruct->strides[C_index]) &&
         (hStride == tensorStruct->strides[H_index]) &&
         (wStride == tensorStruct->strides[W_index]);
}

EXAPI size_t etdnnGetVersion() { return ETDNN_VERSION; }

EXAPI etdnnStatus_t etdnnCreate(etdnnHandle_t *handle) {
  EtDnnContext *ctx = new EtDnnContext();
  *handle = (etdnnHandle_t)ctx;
  return ETDNN_STATUS_SUCCESS;
}

EXAPI etdnnStatus_t etdnnDestroy(etdnnHandle_t handle) {
  EtDnnContext *ctx = (EtDnnContext *)handle;
  delete ctx;
  return ETDNN_STATUS_SUCCESS;
}

EXAPI etdnnStatus_t etdnnSetStream(etdnnHandle_t handle,
                                   etrtStream_t streamId) {
  EtDnnContext *ctx = (EtDnnContext *)handle;

  GetDev dev;

  ctx->etStream = dev->getStream(streamId);
  return ETDNN_STATUS_SUCCESS;
}

EXAPI etdnnStatus_t etdnnGetStream(etdnnHandle_t handle,
                                   etrtStream_t *streamId) {
  EtDnnContext *ctx = (EtDnnContext *)handle;

  *streamId = reinterpret_cast<etrtStream_t>(ctx->etStream);
  return ETDNN_STATUS_SUCCESS;
}

/**
 * Implementation of cudnnCreateTensorDescriptor()
 */
EXAPI etdnnStatus_t
etdnnCreateTensorDescriptor(etdnnTensorDescriptor_t *tensorDesc) {
  EtDnnTensorStruct *ts = new EtDnnTensorStruct();
  *tensorDesc = reinterpret_cast<etdnnTensorDescriptor_t>(ts);
  return ETDNN_STATUS_SUCCESS;
}

/**
 * Implementation of cudnnDestroyTensorDescriptor()
 */
EXAPI etdnnStatus_t
etdnnDestroyTensorDescriptor(etdnnTensorDescriptor_t tensorDesc) {
  EtDnnTensorStruct *ts = (EtDnnTensorStruct *)tensorDesc;
  delete ts;
  return ETDNN_STATUS_SUCCESS;
}

/**
 * Implementation of cudnnSetTensor4dDescriptorEx
 *
 * In contradistinction to cudnnSetTensor4dDescriptor this function receives
 * explicitly passed stride values
 */
EXAPI etdnnStatus_t etdnnSetTensor4dDescriptorEx(
    etdnnTensorDescriptor_t tensorDesc, etdnnDataType_t dataType, int n, int c,
    int h, int w, int nStride, int cStride, int hStride, int wStride) {
  EtDnnTensorStruct *ts = (EtDnnTensorStruct *)tensorDesc;
  ts->dataType = dataType;
  ts->dimensions = 4;

  ts->sizes[N_index] = n;
  ts->sizes[C_index] = c;
  ts->sizes[H_index] = h;
  ts->sizes[W_index] = w;

  ts->strides[N_index] = nStride;
  ts->strides[C_index] = cStride;
  ts->strides[H_index] = hStride;
  ts->strides[W_index] = wStride;

  return ETDNN_STATUS_SUCCESS;
}

/**
 * Implementation of cudnnSetTensor4dDescriptor.
 * Stride values depend on n,c,h,w, and format
 */
EXAPI etdnnStatus_t etdnnSetTensor4dDescriptor(
    etdnnTensorDescriptor_t tensorDesc, etdnnTensorFormat_t format,
    etdnnDataType_t dataType, int n, int c, int h, int w) {
  int nStride, cStride, hStride, wStride;
  switch (format) {
  case ETDNN_TENSOR_NCHW:
    wStride = 1;
    hStride = w;
    cStride = w * h;
    nStride = w * h * c;
    break;
  case ETDNN_TENSOR_NHWC:
    cStride = 1;
    wStride = c;
    hStride = c * w;
    nStride = c * w * h;
    break;
  case ETDNN_TENSOR_NCHW_VECT_C:
  default:
    // not implemented yet
    abort();
  }

  return etdnnSetTensor4dDescriptorEx(tensorDesc, dataType, n, c, h, w, nStride,
                                      cStride, hStride, wStride);
}

/**
 * Implementation of cudnnSetTensorNdDescriptor
 */
EXAPI etdnnStatus_t etdnnSetTensorNdDescriptor(
    etdnnTensorDescriptor_t tensorDesc, etdnnDataType_t dataType, int nbDims,
    const int *dimA, const int *strideA) {
  if (nbDims < 4 || nbDims > ETDNN_DIM_MAX) {
    return ETDNN_STATUS_NOT_SUPPORTED;
  }

  // NOTE: need to check correctness for dim > 4. For now there's no appropriate
  // test case.
  if (nbDims != 4) {
    abort();
  }

  EtDnnTensorStruct *ts = (EtDnnTensorStruct *)tensorDesc;
  ts->dataType = dataType;
  ts->dimensions = nbDims;

  for (int i = 0; i < nbDims; i++) {
    ts->sizes[i] = dimA[i];
    ts->strides[i] = strideA[i];
  }
  return ETDNN_STATUS_SUCCESS;
}

/**
 * Implementation of cudnnSetTensorNdDescriptorEx
 */
EXAPI etdnnStatus_t etdnnSetTensorNdDescriptorEx(
    etdnnTensorDescriptor_t tensorDesc, etdnnTensorFormat_t format,
    etdnnDataType_t dataType, int nbDims, const int *dimA) {
  // There's some doubt with this function. Better leave in unimplemented until
  // we meet it's call in real test case.
  abort();
}

EXAPI etdnnStatus_t etdnnSetTensorQuantizationParams(
    etdnnTensorDescriptor_t tensorDesc, float scale, int32_t offset) {
  EtDnnTensorStruct *ts = (EtDnnTensorStruct *)tensorDesc;
  ts->scale = scale;
  ts->offset = offset;
  return ETDNN_STATUS_SUCCESS;
}

/**
 * Implementation of cudnnCreateFilterDescriptor
 */
EXAPI etdnnStatus_t
etdnnCreateFilterDescriptor(etdnnFilterDescriptor_t *filterDesc) {
  EtDnnFilterStruct *fs = new EtDnnFilterStruct();
  *filterDesc = reinterpret_cast<etdnnFilterDescriptor_t>(fs);
  return ETDNN_STATUS_SUCCESS;
}

/**
 * Implementation of cudnnDestroyFilterDescriptor
 */
EXAPI etdnnStatus_t
etdnnDestroyFilterDescriptor(etdnnFilterDescriptor_t filterDesc) {
  EtDnnFilterStruct *fs = (EtDnnFilterStruct *)filterDesc;
  delete fs;
  return ETDNN_STATUS_SUCCESS;
}

/**
 * Implementation of cudnnSetFilter4dDescriptor
 */
EXAPI etdnnStatus_t etdnnSetFilter4dDescriptor(
    etdnnFilterDescriptor_t filterDesc, etdnnDataType_t dataType,
    etdnnTensorFormat_t format, int k, int c, int h, int w) {
  EtDnnFilterStruct *fs = (EtDnnFilterStruct *)filterDesc;
  fs->dataType = dataType;
  fs->format = format;
  fs->k = k;
  fs->c = c;
  fs->h = h;
  fs->w = w;
  return ETDNN_STATUS_SUCCESS;
}

EXAPI etdnnStatus_t etdnnSetFilterQuantizationParams(
    etdnnFilterDescriptor_t filterDesc, float scale, int32_t offset) {
  EtDnnFilterStruct *fs = (EtDnnFilterStruct *)filterDesc;
  fs->scale = scale;
  fs->offset = offset;
  return ETDNN_STATUS_SUCCESS;
}

/**
 * Implementation of cudnnCreateConvolutionDescriptor
 */
EXAPI etdnnStatus_t
etdnnCreateConvolutionDescriptor(etdnnConvolutionDescriptor_t *convDesc) {
  EtDnnConvolutionStruct *cs = new EtDnnConvolutionStruct();
  *convDesc = reinterpret_cast<etdnnConvolutionDescriptor_t>(cs);
  return ETDNN_STATUS_SUCCESS;
}

/**
 * Implementation of cudnnDestroyConvolutionDescriptor
 */
EXAPI etdnnStatus_t
etdnnDestroyConvolutionDescriptor(etdnnConvolutionDescriptor_t convDesc) {
  EtDnnConvolutionStruct *cs = (EtDnnConvolutionStruct *)convDesc;
  delete cs;
  return ETDNN_STATUS_SUCCESS;
}

/**
 * Implementation of cudnnSetConvolution2dDescriptor
 */
EXAPI etdnnStatus_t etdnnSetConvolution2dDescriptor(
    etdnnConvolutionDescriptor_t convDesc, int pad_h, int pad_w, int u, int v,
    int dilation_h, int dilation_w, etdnnConvolutionMode_t mode,
    etdnnDataType_t computeType) {
  EtDnnConvolutionStruct *cs = (EtDnnConvolutionStruct *)convDesc;
  cs->pad_h = pad_h;
  cs->pad_w = pad_w;
  cs->u = u;
  cs->v = v;
  cs->dilation_h = dilation_h;
  cs->dilation_w = dilation_w;
  cs->mode = mode;
  cs->computeType = computeType;
  cs->groupCount = 1;
  return ETDNN_STATUS_SUCCESS;
}

EXAPI etdnnStatus_t etdnnSetConvolutionGroupCount(
    etdnnConvolutionDescriptor_t convDesc, int groupCount) {
  EtDnnConvolutionStruct *conv = (EtDnnConvolutionStruct *)convDesc;
  conv->groupCount = groupCount;
  return ETDNN_STATUS_SUCCESS;
}

EXAPI etdnnStatus_t etdnnSetConvolutionBiasQuantizationParams(
    etdnnConvolutionDescriptor_t convDesc, float scale, int32_t offset) {
  EtDnnConvolutionStruct *conv = (EtDnnConvolutionStruct *)convDesc;
  conv->biasScale = scale;
  conv->biasOffset = offset;
  return ETDNN_STATUS_SUCCESS;
}

/**
 * Implementation of cudnnCreateActivationDescriptor()
 */
EXAPI etdnnStatus_t
etdnnCreateActivationDescriptor(etdnnActivationDescriptor_t *activationDesc) {
  EtDnnActivationStruct *as = new EtDnnActivationStruct();
  *activationDesc = reinterpret_cast<etdnnActivationDescriptor_t>(as);
  return ETDNN_STATUS_SUCCESS;
}

/**
 * Implementation of cudnnDestroyActivationDescriptor()
 */
EXAPI etdnnStatus_t
etdnnDestroyActivationDescriptor(etdnnActivationDescriptor_t activationDesc) {
  EtDnnActivationStruct *as = (EtDnnActivationStruct *)activationDesc;
  delete as;
  return ETDNN_STATUS_SUCCESS;
}

/**
 * Implementation of cudnnSetActivationDescriptor()
 */
EXAPI etdnnStatus_t etdnnSetActivationDescriptor(
    etdnnActivationDescriptor_t activationDesc, etdnnActivationMode_t mode,
    etdnnNanPropagation_t reluNanOpt, double coef) {
  EtDnnActivationStruct *as = (EtDnnActivationStruct *)activationDesc;
  as->mode = mode;
  as->nan_prop = reluNanOpt;
  as->coef = coef;
  return ETDNN_STATUS_SUCCESS;
}

/**
 * Implementation of cudnnCreatePoolingDescriptor()
 */
EXAPI etdnnStatus_t
etdnnCreatePoolingDescriptor(etdnnPoolingDescriptor_t *poolingDesc) {
  EtDnnPoolingStruct *ps = new EtDnnPoolingStruct();
  *poolingDesc = reinterpret_cast<etdnnPoolingDescriptor_t>(ps);
  return ETDNN_STATUS_SUCCESS;
}

/**
 * Implementation of cudnnDestroyPoolingDescriptor()
 */
EXAPI etdnnStatus_t
etdnnDestroyPoolingDescriptor(etdnnPoolingDescriptor_t poolingDesc) {
  EtDnnPoolingStruct *ps = (EtDnnPoolingStruct *)poolingDesc;
  delete ps;
  return ETDNN_STATUS_SUCCESS;
}

/**
 * Implementation of cudnnSetPooling2dDescriptor()
 */
EXAPI etdnnStatus_t etdnnSetPooling2dDescriptor(
    etdnnPoolingDescriptor_t poolingDesc, etdnnPoolingMode_t mode,
    etdnnNanPropagation_t maxpoolingNanOpt, int windowHeight, int windowWidth,
    int verticalPadding, int horizontalPadding, int verticalStride,
    int horizontalStride) {
  EtDnnPoolingStruct *ps = (EtDnnPoolingStruct *)poolingDesc;
  ps->mode = mode;
  ps->nan_prop = maxpoolingNanOpt;
  ps->window_h = windowHeight;
  ps->window_w = windowWidth;
  ps->vertical_pad = verticalPadding;
  ps->horizontal_pad = horizontalPadding;
  ps->vertical_stride = verticalStride;
  ps->horizontal_stride = horizontalStride;

  return ETDNN_STATUS_SUCCESS;
}

/**
 * Implementation of cudnnActivationForward()
 */
EXAPI etdnnStatus_t etdnnActivationForward(
    etdnnHandle_t handle, etdnnActivationDescriptor_t activationDesc,
    const void *alpha, const etdnnTensorDescriptor_t xDesc, const void *x,
    const void *beta, const etdnnTensorDescriptor_t yDesc, void *y) {
  EtDnnContext *ctx = (EtDnnContext *)handle;
  EtDnnTensorStruct *xt = (EtDnnTensorStruct *)xDesc;
  EtDnnTensorStruct *yt = (EtDnnTensorStruct *)yDesc;
  EtDnnActivationStruct *ad = (EtDnnActivationStruct *)activationDesc;

  if (xt->dataType != yt->dataType || xt->dimensions != yt->dimensions) {
    return ETDNN_STATUS_BAD_PARAM;
  }

  for (int i = 0; i < xt->dimensions; i++) {
    if (xt->sizes[i] != yt->sizes[i]) {
      return ETDNN_STATUS_BAD_PARAM;
    }
  }

  /* Currently support only 4 dimension version */
  assert(xt->dimensions == 4);

  {
    GetDev dev;

    assert(dev->isPtrAllocedDev(x));
    assert(dev->isPtrAllocedDev(y));
  }

  ActivationForward_KernelArgs_t args_struct = {ad->mode,
                                                ad->nan_prop,
                                                (float)ad->coef,
                                                xt->dataType,
                                                xt->dimensions,
                                                xt->sizes[N_index],
                                                xt->sizes[C_index],
                                                xt->sizes[H_index],
                                                xt->sizes[W_index],
                                                xt->strides[N_index],
                                                xt->strides[C_index],
                                                xt->strides[H_index],
                                                xt->strides[W_index],
                                                yt->strides[N_index],
                                                yt->strides[C_index],
                                                yt->strides[H_index],
                                                yt->strides[W_index],
                                                x,
                                                y,
                                                {0},
                                                {0}};

  std::string kernel_name = "ActivationForward";

  switch (args_struct.ac_function) {
  case ETDNN_ACTIVATION_SIGMOID:
    kernel_name = kernel_name + "_Sigmoid";
    break;
  case ETDNN_ACTIVATION_RELU:
    kernel_name = kernel_name + "_Relu";
    break;
  case ETDNN_ACTIVATION_TANH:
    kernel_name = kernel_name + "_Tanh";
    break;
  default:
    abort();
    break;
  }

  kernel_name = setArgsAlphaBetaAndFixKernelName(
      &args_struct, kernel_name, args_struct.data_type, alpha, beta);

  dim3 gridDim(defaultGridDim1D(args_struct.n * args_struct.c * args_struct.h *
                                args_struct.w));
  dim3 blockDim(defaultBlockDim1D());
  etrtConfigureCall(gridDim, blockDim, 0,
                    reinterpret_cast<etrtStream_t>(ctx->etStream));
  etrtSetupArgument(&args_struct, sizeof(args_struct), 0);
  etrtLaunch(nullptr, kernel_name.c_str());
  return ETDNN_STATUS_SUCCESS;
}

EXAPI etdnnStatus_t etdnnActivationLinear(etdnnHandle_t handle,
                                          etdnnActivationMode_t mode,
                                          const etdnnTensorDescriptor_t xDesc,
                                          const void *x,
                                          const etdnnTensorDescriptor_t yDesc,
                                          void *y) {
  EtDnnContext *ctx = (EtDnnContext *)handle;
  EtDnnTensorStruct *xt = (EtDnnTensorStruct *)xDesc;
  EtDnnTensorStruct *yt = (EtDnnTensorStruct *)yDesc;

  if (yt->dimensions != 4 || xt->dimensions != yt->dimensions ||
      xt->dataType != yt->dataType) {
    abort();
  }

  for (int i = 0; i < xt->dimensions; i++) {
    if (xt->sizes[i] != yt->sizes[i]) {
      abort();
    }
  }

  {
    GetDev dev;

    assert(dev->isPtrAllocedDev(x));
    assert(dev->isPtrAllocedDev(y));
  }

  std::string kernel_name;

  switch (mode) {
  case ETDNN_ACTIVATION_SIGMOID:
    kernel_name = "SigmoidKernel";
    break;
  case ETDNN_ACTIVATION_TANH:
    kernel_name = "TanhKernel";
    break;
  default:
    abort();
    break;
  }

  kernel_name = fixKernelNameByDataType(kernel_name, yt->dataType);

  int len = yt->sizes[N_index] * yt->sizes[C_index] * yt->sizes[H_index] *
            yt->sizes[W_index];
  dim3 gridDim(defaultGridDim1D(len));
  dim3 blockDim(defaultBlockDim1D());
  etrtConfigureCall(gridDim, blockDim, 0,
                    reinterpret_cast<etrtStream_t>(ctx->etStream));

  etrtSetupArgument(&len, 4, 0);
  etrtSetupArgument(&x, 8, 8);
  etrtSetupArgument(&y, 8, 16);
  etrtLaunch(nullptr, kernel_name.c_str());
  return ETDNN_STATUS_SUCCESS;
}

/**
 * Implementation of cudnnPoolingForward()
 */
EXAPI etdnnStatus_t etdnnPoolingForward(
    etdnnHandle_t handle, etdnnPoolingDescriptor_t poolingDesc,
    const void *alpha, const etdnnTensorDescriptor_t xDesc, const void *x,
    const void *beta, const etdnnTensorDescriptor_t yDesc, void *y) {
  EtDnnContext *ctx = (EtDnnContext *)handle;
  EtDnnTensorStruct *xt = (EtDnnTensorStruct *)xDesc;
  EtDnnTensorStruct *yt = (EtDnnTensorStruct *)yDesc;
  EtDnnPoolingStruct *ps = (EtDnnPoolingStruct *)poolingDesc;

  if (xt->dataType != yt->dataType || xt->dimensions != yt->dimensions ||
      xt->sizes[N_index] != yt->sizes[N_index] ||
      xt->sizes[C_index] != yt->sizes[C_index]) {
    return ETDNN_STATUS_BAD_PARAM;
  }

  //    if ( xt->strides[W_index] != 1
  //         || yt->strides[W_index] != 1)
  //    {
  //        return ETDNN_STATUS_NOT_SUPPORTED;
  //    }

  /* Currently support only 4 dimension version */
  assert(xt->dimensions == 4);

  {
    GetDev dev;

    assert(dev->isPtrAllocedDev(x));
    assert(dev->isPtrAllocedDev(y));
  }

  PoolingForward_KernelArgs_t args_struct = {ps->mode,
                                             ps->nan_prop,
                                             ps->window_h,
                                             ps->window_w,
                                             ps->vertical_pad,
                                             ps->horizontal_pad,
                                             ps->vertical_stride,
                                             ps->horizontal_stride,
                                             xt->dataType,
                                             xt->dimensions,
                                             xt->sizes[N_index],
                                             xt->sizes[C_index],
                                             xt->sizes[H_index],
                                             xt->sizes[W_index],
                                             xt->strides[N_index],
                                             xt->strides[C_index],
                                             xt->strides[H_index],
                                             xt->strides[W_index],
                                             yt->sizes[N_index],
                                             yt->sizes[C_index],
                                             yt->sizes[H_index],
                                             yt->sizes[W_index],
                                             yt->strides[N_index],
                                             yt->strides[C_index],
                                             yt->strides[H_index],
                                             yt->strides[W_index],
                                             x,
                                             y,
                                             {0},
                                             {0},
                                             xt->scale,
                                             xt->offset,
                                             yt->scale,
                                             yt->offset};

  std::string kernel_name = "PoolingForward";

  switch (args_struct.pooling_function) {
  case ETDNN_POOLING_MAX:
    kernel_name = kernel_name + "_Max";
    break;
  case ETDNN_POOLING_AVERAGE_COUNT_INCLUDE_PADDING:
    kernel_name = kernel_name + "_AvgIn";
    break;
  case ETDNN_POOLING_AVERAGE_COUNT_EXCLUDE_PADDING:
    kernel_name = kernel_name + "_AvgEx";
    break;
  default:
    abort();
    break;
  }

  kernel_name = setArgsAlphaBetaAndFixKernelName(
      &args_struct, kernel_name, args_struct.data_type, alpha, beta);

  dim3 gridDim(defaultGridDim1D(args_struct.out_n * args_struct.out_c *
                                args_struct.out_h * args_struct.out_w));
  dim3 blockDim(defaultBlockDim1D());
  etrtConfigureCall(gridDim, blockDim, 0,
                    reinterpret_cast<etrtStream_t>(ctx->etStream));
  etrtSetupArgument(&args_struct, sizeof(args_struct), 0);
  etrtLaunch(nullptr, kernel_name.c_str());
  return ETDNN_STATUS_SUCCESS;
}

/**
 * Implementation of cudnnAddTensor
 */
EXAPI etdnnStatus_t etdnnAddTensor(etdnnHandle_t handle, const void *alpha,
                                   const etdnnTensorDescriptor_t aDesc,
                                   const void *A, const void *beta,
                                   const etdnnTensorDescriptor_t cDesc,
                                   void *C) {
  EtDnnContext *ctx = (EtDnnContext *)handle;
  EtDnnTensorStruct *at = (EtDnnTensorStruct *)aDesc;
  EtDnnTensorStruct *ct = (EtDnnTensorStruct *)cDesc;

  if (at->dataType != ct->dataType || at->dimensions != ct->dimensions ||
      at->dimensions < 1) {
    return ETDNN_STATUS_BAD_PARAM;
  }

  if (at->dimensions > 5) {
    return ETDNN_STATUS_NOT_SUPPORTED;
  }

  for (int i = 0; i < at->dimensions; i++) {
    if (at->sizes[i] != ct->sizes[i] && at->sizes[i] != 1) {
      return ETDNN_STATUS_BAD_PARAM;
    }
  }

  /* Currently support only 4 dimension version */
  assert(at->dimensions == 4);

  {
    GetDev dev;

    assert(dev->isPtrAllocedDev(A));
    assert(dev->isPtrAllocedDev(C));
  }

  AddTensor_KernelArgs_t args_struct = {at->dataType,
                                        at->dimensions,
                                        at->sizes[N_index],
                                        at->sizes[C_index],
                                        at->sizes[H_index],
                                        at->sizes[W_index],
                                        0,
                                        ct->sizes[N_index],
                                        ct->sizes[C_index],
                                        ct->sizes[H_index],
                                        ct->sizes[W_index],
                                        0,
                                        at->strides[N_index],
                                        at->strides[C_index],
                                        at->strides[H_index],
                                        at->strides[W_index],
                                        0,
                                        ct->strides[N_index],
                                        ct->strides[C_index],
                                        ct->strides[H_index],
                                        ct->strides[W_index],
                                        0,
                                        A,
                                        C,
                                        {0},
                                        {0}};

  std::string kernel_name = setArgsAlphaBetaAndFixKernelName(
      &args_struct, "AddTensor", args_struct.data_type, alpha, beta);

  dim3 gridDim(defaultGridDim1D(args_struct.out_n * args_struct.out_c *
                                args_struct.out_h * args_struct.out_w));
  dim3 blockDim(defaultBlockDim1D());
  etrtConfigureCall(gridDim, blockDim, 0,
                    reinterpret_cast<etrtStream_t>(ctx->etStream));
  etrtSetupArgument(&args_struct, sizeof(args_struct), 0);
  etrtLaunch(nullptr, kernel_name.c_str());
  return ETDNN_STATUS_SUCCESS;
}

EXAPI etdnnStatus_t etdnnDeriveBNTensorDescriptor(
    etdnnTensorDescriptor_t derivedBnDesc, const etdnnTensorDescriptor_t xDesc,
    etdnnBatchNormMode_t mode) {
  EtDnnTensorStruct *tx = (EtDnnTensorStruct *)xDesc;

  // FIXME: support of 5 dimensions
  assert(tx->dimensions == 4);

  etdnnDataType_t derived_type =
      (tx->dataType == ETDNN_DATA_HALF) ? ETDNN_DATA_FLOAT : tx->dataType;

  switch (mode) {
  case ETDNN_BATCHNORM_PER_ACTIVATION:
    etdnnSetTensor4dDescriptor(derivedBnDesc, ETDNN_TENSOR_NCHW, derived_type,
                               1, tx->sizes[C_index], tx->sizes[H_index],
                               tx->sizes[W_index]);
    break;
  case ETDNN_BATCHNORM_SPATIAL:
  case ETDNN_BATCHNORM_SPATIAL_PERSISTENT:
    etdnnSetTensor4dDescriptor(derivedBnDesc, ETDNN_TENSOR_NCHW, derived_type,
                               1, tx->sizes[C_index], 1, 1);
    break;
  default:
    return ETDNN_STATUS_BAD_PARAM;
  }

  return ETDNN_STATUS_SUCCESS;
}

EXAPI etdnnStatus_t etdnnBatchNormalizationForwardInference(
    etdnnHandle_t handle, etdnnBatchNormMode_t mode, const void *alpha,
    const void *beta, const etdnnTensorDescriptor_t xDesc, const void *x,
    const etdnnTensorDescriptor_t yDesc, void *y,
    const etdnnTensorDescriptor_t bnScaleBiasMeanVarDesc, const void *bnScale,
    const void *bnBias, const void *estimatedMean,
    const void *estimatedVariance, double epsilon) {
  EtDnnContext *ctx = (EtDnnContext *)handle;
  EtDnnTensorStruct *xt = (EtDnnTensorStruct *)xDesc;
  EtDnnTensorStruct *yt = (EtDnnTensorStruct *)yDesc;
  EtDnnTensorStruct *bnt = (EtDnnTensorStruct *)bnScaleBiasMeanVarDesc;

  if (xt->dataType != yt->dataType || xt->dimensions != yt->dimensions ||
      xt->dimensions != bnt->dimensions) {
    return ETDNN_STATUS_BAD_PARAM;
  }

  if (xt->dimensions != 4 && xt->dimensions != 5) {
    return ETDNN_STATUS_NOT_SUPPORTED;
  }

  for (int i = 0; i < xt->dimensions; i++) {
    if (xt->sizes[i] != yt->sizes[i]) {
      return ETDNN_STATUS_BAD_PARAM;
    }
  }

  if (bnt->sizes[N_index] != 1 || bnt->sizes[C_index] != xt->sizes[C_index]) {
    return ETDNN_STATUS_BAD_PARAM;
  }

  if (mode == ETDNN_BATCHNORM_PER_ACTIVATION) {
    if (bnt->sizes[H_index] != xt->sizes[H_index] ||
        bnt->sizes[W_index] != xt->sizes[W_index]) {
      return ETDNN_STATUS_BAD_PARAM;
    }
  } else {
    if (bnt->sizes[H_index] != 1 || bnt->sizes[W_index] != 1) {
      return ETDNN_STATUS_BAD_PARAM;
    }
  }

  if (epsilon < ETDNN_BN_MIN_EPSILON) {
    return ETDNN_STATUS_BAD_PARAM;
  }

  // FIXME: support of 5 dimensions
  assert(xt->dimensions == 4);

  {
    GetDev dev;

    assert(dev->isPtrAllocedDev(x));
    assert(dev->isPtrAllocedDev(y));
    assert(dev->isPtrAllocedDev(bnScale));
    assert(dev->isPtrAllocedDev(bnBias));
    assert(dev->isPtrAllocedDev(estimatedMean));
    assert(dev->isPtrAllocedDev(estimatedVariance));
  }

  BatchNormalizationForwardInference_KernelArgs_t args_struct = {
      xt->dataType,
      bnt->dataType,
      xt->dimensions,
      mode,
      xt->sizes[N_index],
      xt->sizes[C_index],
      xt->sizes[H_index],
      xt->sizes[W_index],
      xt->strides[N_index],
      xt->strides[C_index],
      xt->strides[H_index],
      xt->strides[W_index],
      yt->strides[N_index],
      yt->strides[C_index],
      yt->strides[H_index],
      yt->strides[W_index],
      bnt->strides[N_index],
      bnt->strides[C_index],
      bnt->strides[H_index],
      bnt->strides[W_index],
      x,
      y,
      bnScale,
      bnBias,
      estimatedMean,
      estimatedVariance,
      {0},
      {0},
      (float)epsilon};

  std::string kernel_name = setArgsAlphaBetaAndFixKernelName(
      &args_struct, "BatchNormalizationForwardInference", args_struct.data_type,
      alpha, beta);

  dim3 gridDim(defaultGridDim1D(args_struct.n * args_struct.c * args_struct.h *
                                args_struct.w));
  dim3 blockDim(defaultBlockDim1D());
  etrtConfigureCall(gridDim, blockDim, 0,
                    reinterpret_cast<etrtStream_t>(ctx->etStream));
  etrtSetupArgument(&args_struct, sizeof(args_struct), 0);
  etrtLaunch(nullptr, kernel_name.c_str());
  return ETDNN_STATUS_SUCCESS;
}

EXAPI etdnnStatus_t etdnnGetConvolutionForwardAlgorithm(
    etdnnHandle_t handle, const etdnnTensorDescriptor_t xDesc,
    const etdnnFilterDescriptor_t wDesc,
    const etdnnConvolutionDescriptor_t convDesc,
    const etdnnTensorDescriptor_t yDesc,
    etdnnConvolutionFwdPreference_t preference, size_t memoryLimitInBytes,
    etdnnConvolutionFwdAlgo_t *algo) {
  // TODO: need to support several algos
  *algo = ETDNN_CONVOLUTION_FWD_ALGO_DIRECT;

  return ETDNN_STATUS_SUCCESS;
}

EXAPI etdnnStatus_t etdnnGetConvolutionForwardWorkspaceSize(
    etdnnHandle_t handle, const etdnnTensorDescriptor_t xDesc,
    const etdnnFilterDescriptor_t wDesc,
    const etdnnConvolutionDescriptor_t convDesc,
    const etdnnTensorDescriptor_t yDesc, etdnnConvolutionFwdAlgo_t algo,
    size_t *sizeInBytes) {
  // EtDnnContext *ctx = (EtDnnContext *)handle;
  EtDnnTensorStruct *xt = (EtDnnTensorStruct *)xDesc;
  EtDnnFilterStruct *wt = (EtDnnFilterStruct *)wDesc;
  EtDnnTensorStruct *yt = (EtDnnTensorStruct *)yDesc;
  // EtDnnConvolutionStruct* conv = (EtDnnConvolutionStruct*)convDesc;

  assert(algo == ETDNN_CONVOLUTION_FWD_ALGO_DIRECT);

  if (xt->dataType == ETDNN_DATA_FLOAT &&
      isTensorInDenseFormat(xt, ETDNN_TENSOR_NHWC) &&
      isTensorInDenseFormat(yt, ETDNN_TENSOR_NHWC) &&
      wt->format == ETDNN_TENSOR_NHWC &&
      is_aligned(xt->sizes[C_index] * sizeof(float), 64) &&
      is_aligned(wt->h * wt->w * wt->k * sizeof(float), 64)) {
    int m = xt->sizes[H_index] * xt->sizes[W_index] * xt->sizes[N_index];
    int n = wt->h * wt->w * wt->k;

    assert(wt->k == yt->sizes[C_index]);
    assert(wt->c == xt->sizes[C_index]);

    size_t neededWorkSpaceSizeInBytes = sizeof(float) * m * n;

    *sizeInBytes = neededWorkSpaceSizeInBytes;
    return ETDNN_STATUS_SUCCESS;
  }

  if (xt->dataType == ETDNN_DATA_INT8Q &&
      isTensorInDenseFormat(xt, ETDNN_TENSOR_NHWC) &&
      isTensorInDenseFormat(yt, ETDNN_TENSOR_NHWC) &&
      wt->format == ETDNN_TENSOR_NHWC && xt->offset == 0 && wt->offset == 0 &&
      (xt->sizes[C_index] % 4) == 0 &&
      is_aligned(xt->sizes[C_index] * sizeof(int8_t), 64) &&
      is_aligned(wt->h * wt->w * wt->k * sizeof(int32_t), 64)) {
    int m = xt->sizes[H_index] * xt->sizes[W_index] * xt->sizes[N_index];
    int n = wt->h * wt->w * wt->k;

    assert(wt->k == yt->sizes[C_index]);
    assert(wt->c == xt->sizes[C_index]);

    size_t neededWorkSpaceSizeInBytes = sizeof(int32_t) * m * n;

    *sizeInBytes = neededWorkSpaceSizeInBytes;
    return ETDNN_STATUS_SUCCESS;
  }

  *sizeInBytes = 0;
  return ETDNN_STATUS_SUCCESS;
}

/*
 * y = α * (W(x) + b) + β * y
 *
 * See also: cpuConvolutionForward
 */
EXAPI etdnnStatus_t etdnnBiasedConvolutionForward(
    etdnnHandle_t handle, const void *alpha,
    const etdnnTensorDescriptor_t xDesc, const void *x,
    const etdnnFilterDescriptor_t wDesc, const void *w,
    const etdnnConvolutionDescriptor_t convDesc, const void *bias,
    etdnnConvolutionFwdAlgo_t algo, void *workSpace,
    size_t workSpaceSizeInBytes, const void *beta,
    const etdnnTensorDescriptor_t yDesc, void *y) {
  EtDnnContext *ctx = (EtDnnContext *)handle;
  EtDnnTensorStruct *xt = (EtDnnTensorStruct *)xDesc;
  EtDnnFilterStruct *wt = (EtDnnFilterStruct *)wDesc;
  EtDnnTensorStruct *yt = (EtDnnTensorStruct *)yDesc;
  EtDnnConvolutionStruct *conv = (EtDnnConvolutionStruct *)convDesc;

  assert(algo == ETDNN_CONVOLUTION_FWD_ALGO_DIRECT);
  assert(xt->dimensions == 4);
  assert(yt->dimensions == 4);
  assert(conv->groupCount == 1); // FIXME: no groups are supported for now

  if (xt->dataType != wt->dataType || yt->dataType != wt->dataType) {
    return ETDNN_STATUS_BAD_PARAM;
  }

  {
    GetDev dev;

    assert(dev->isPtrAllocedDev(x));
    assert(dev->isPtrAllocedDev(w));
    assert(dev->isPtrAllocedDev(y));

    if (bias) {
      assert(dev->isPtrAllocedDev(bias));
    }

    if (workSpace) {
      assert(dev->isPtrAllocedDev(workSpace));
    }
  }

  if (wt->h == 1 && wt->w == 1 // Optimization for 1x1 filter size
      && xt->dataType == ETDNN_DATA_FLOAT && (*(float *)alpha) == 1.0f &&
      (*(float *)beta) == 0.0f && conv->pad_h == 0 && conv->pad_w == 0 &&
      conv->dilation_h == 1 && conv->dilation_w == 1 && conv->u == 1 &&
      conv->v == 1 && isTensorInDenseFormat(xt, ETDNN_TENSOR_NHWC) &&
      isTensorInDenseFormat(yt, ETDNN_TENSOR_NHWC) &&
      wt->format == ETDNN_TENSOR_NHWC && is_aligned((uintptr_t)x, 64) &&
      is_aligned((uintptr_t)w, 64) && is_aligned((uintptr_t)y, 64) &&
      is_aligned(xt->sizes[C_index] * sizeof(float), 64) &&
      is_aligned(wt->k * sizeof(float), 64)) {
    int m = xt->sizes[H_index] * xt->sizes[W_index] * xt->sizes[N_index];
    int n = wt->k;
    int k = xt->sizes[C_index];

    etiMatMulTensor(reinterpret_cast<etrtStream_t>(ctx->etStream), m, n, k,
                    (const float *)x, k, false, (const float *)w, k, true,
                    (float *)y, n);

    if (bias != 0) {
      etiBatchedAdd(reinterpret_cast<etrtStream_t>(ctx->etStream),
                    yt->sizes[N_index] * yt->sizes[H_index] *
                        yt->sizes[W_index],
                    yt->sizes[C_index], (float *)y, (float *)bias, (float *)y);
    }
    return ETDNN_STATUS_SUCCESS;
  }

  // Optimization not only for 1x1 filter size, but requires workspace
  if (xt->dataType == ETDNN_DATA_FLOAT && (*(float *)alpha) == 1.0f &&
      (*(float *)beta) == 0.0f &&
      isTensorInDenseFormat(xt, ETDNN_TENSOR_NHWC) &&
      isTensorInDenseFormat(yt, ETDNN_TENSOR_NHWC) &&
      wt->format == ETDNN_TENSOR_NHWC && is_aligned((uintptr_t)x, 64) &&
      is_aligned((uintptr_t)w, 64) && is_aligned((uintptr_t)y, 64) &&
      is_aligned((uintptr_t)workSpace, 64) &&
      is_aligned(xt->sizes[C_index] * sizeof(float), 64) &&
      is_aligned(wt->h * wt->w * wt->k * sizeof(float), 64)) {
    /*
     * Represent convolution in two steps:
     * 1. MatMul into workspace, which caches all pairwise dot products of
     * feature maps,
     * 2. ConvTail with rest of calculations.
     *
     * MatMul explanation: A * Bᵀ = C
     * A - input fully packed NHWC tensor treated as matrix m x k
     * Bᵀ - filter fully packed NHWC tensor treated as matrix k x n
     * C - intermediate matrix m x n with cached pairwise dot products of
     * feature maps.
     */
    int m = xt->sizes[H_index] * xt->sizes[W_index] * xt->sizes[N_index];
    int n = wt->h * wt->w * wt->k;
    int k = xt->sizes[C_index];

    assert(wt->k == yt->sizes[C_index]);
    assert(wt->c == xt->sizes[C_index]);

    size_t neededWorkSpaceSizeInBytes = sizeof(float) * m * n;

    if (neededWorkSpaceSizeInBytes <= workSpaceSizeInBytes) {
      etiMatMulTensor(reinterpret_cast<etrtStream_t>(ctx->etStream), m, n, k,
                      (const float *)x, k, false, (const float *)w, k, true,
                      (float *)workSpace, n);

      assert(xt->sizes[N_index] == yt->sizes[N_index]);

      ConvTailNHWC_KernelArgs_t args_struct = {wt->h,
                                               wt->w,
                                               xt->sizes[C_index],
                                               xt->sizes[H_index],
                                               xt->sizes[W_index],
                                               yt->sizes[C_index],
                                               yt->sizes[H_index],
                                               yt->sizes[W_index],
                                               yt->sizes[N_index],
                                               conv->pad_h,
                                               conv->pad_w,
                                               conv->u,
                                               conv->v,
                                               conv->dilation_h,
                                               conv->dilation_w,
                                               workSpace,
                                               bias,
                                               y,
                                               xt->scale,
                                               xt->offset,
                                               wt->scale,
                                               wt->offset,
                                               conv->biasScale,
                                               conv->biasOffset,
                                               yt->scale,
                                               yt->offset};
      dim3 gridDim(defaultGridDim1D(args_struct.batch_size * args_struct.out_c *
                                    args_struct.out_h * args_struct.out_w));
      dim3 blockDim(defaultBlockDim1D());
      etrtConfigureCall(gridDim, blockDim, 0,
                        reinterpret_cast<etrtStream_t>(ctx->etStream));
      etrtSetupArgument(&args_struct, sizeof(args_struct), 0);
      etrtLaunch(nullptr, "ConvTailNHWC_Float");

      return ETDNN_STATUS_SUCCESS;
    }
  }

  // Optimization for Int8Q, requires workspace
  if (xt->dataType == ETDNN_DATA_INT8Q && (*(float *)alpha) == 1.0f &&
      (*(float *)beta) == 0.0f &&
      isTensorInDenseFormat(xt, ETDNN_TENSOR_NHWC) &&
      isTensorInDenseFormat(yt, ETDNN_TENSOR_NHWC) &&
      wt->format == ETDNN_TENSOR_NHWC && xt->offset == 0 && wt->offset == 0 &&
      (xt->sizes[C_index] % 4) == 0 && is_aligned((uintptr_t)x, 64) &&
      is_aligned((uintptr_t)w, 64) && is_aligned((uintptr_t)y, 64) &&
      is_aligned((uintptr_t)workSpace, 64) &&
      is_aligned(xt->sizes[C_index] * sizeof(int8_t), 64) &&
      is_aligned(wt->h * wt->w * wt->k * sizeof(int32_t), 64)) {
    int m = xt->sizes[H_index] * xt->sizes[W_index] * xt->sizes[N_index];
    int n = wt->h * wt->w * wt->k;
    int k = xt->sizes[C_index];

    assert(wt->k == yt->sizes[C_index]);
    assert(wt->c == xt->sizes[C_index]);

    size_t neededWorkSpaceSizeInBytes = sizeof(int32_t) * m * n;

    if (neededWorkSpaceSizeInBytes <= workSpaceSizeInBytes) {
      etiMatMulTensorInt8To32(reinterpret_cast<etrtStream_t>(ctx->etStream), m,
                              n, k, (const int8_t *)x, k, false,
                              (const int8_t *)w, k, true, (int32_t *)workSpace,
                              n);

      assert(xt->sizes[N_index] == yt->sizes[N_index]);

      ConvTailNHWC_KernelArgs_t args_struct = {wt->h,
                                               wt->w,
                                               xt->sizes[C_index],
                                               xt->sizes[H_index],
                                               xt->sizes[W_index],
                                               yt->sizes[C_index],
                                               yt->sizes[H_index],
                                               yt->sizes[W_index],
                                               yt->sizes[N_index],
                                               conv->pad_h,
                                               conv->pad_w,
                                               conv->u,
                                               conv->v,
                                               conv->dilation_h,
                                               conv->dilation_w,
                                               workSpace,
                                               bias,
                                               y,
                                               xt->scale,
                                               xt->offset,
                                               wt->scale,
                                               wt->offset,
                                               conv->biasScale,
                                               conv->biasOffset,
                                               yt->scale,
                                               yt->offset};
      dim3 gridDim(defaultGridDim1D(args_struct.batch_size * args_struct.out_c *
                                    args_struct.out_h * args_struct.out_w));
      dim3 blockDim(defaultBlockDim1D());
      etrtConfigureCall(gridDim, blockDim, 0,
                        reinterpret_cast<etrtStream_t>(ctx->etStream));
      etrtSetupArgument(&args_struct, sizeof(args_struct), 0);
      etrtLaunch(nullptr, "ConvTailNHWC_Int8Q");

      return ETDNN_STATUS_SUCCESS;
    }
  }

  ConvolutionForward_KernelArgs_t args_struct = {xt->dataType,
                                                 wt->format,
                                                 wt->k,
                                                 wt->c,
                                                 wt->h,
                                                 wt->w,
                                                 xt->sizes[N_index],
                                                 xt->sizes[C_index],
                                                 xt->sizes[H_index],
                                                 xt->sizes[W_index],
                                                 xt->strides[N_index],
                                                 xt->strides[C_index],
                                                 xt->strides[H_index],
                                                 xt->strides[W_index],
                                                 yt->sizes[N_index],
                                                 yt->sizes[C_index],
                                                 yt->sizes[H_index],
                                                 yt->sizes[W_index],
                                                 yt->strides[N_index],
                                                 yt->strides[C_index],
                                                 yt->strides[H_index],
                                                 yt->strides[W_index],
                                                 conv->pad_h,
                                                 conv->pad_w,
                                                 conv->u,
                                                 conv->v,
                                                 conv->dilation_h,
                                                 conv->dilation_w,
                                                 workSpace,
                                                 workSpaceSizeInBytes,
                                                 x,
                                                 w,
                                                 bias,
                                                 y,
                                                 {0},
                                                 {0},
                                                 xt->scale,
                                                 xt->offset,
                                                 wt->scale,
                                                 wt->offset,
                                                 conv->biasScale,
                                                 conv->biasOffset,
                                                 yt->scale,
                                                 yt->offset};

  std::string kernel_name = setArgsAlphaBetaAndFixKernelName(
      &args_struct, "ConvolutionForward", args_struct.data_type, alpha, beta);

  dim3 gridDim(defaultGridDim1D(args_struct.out_n * args_struct.out_c *
                                args_struct.out_h * args_struct.out_w));
  dim3 blockDim(defaultBlockDim1D());
  etrtConfigureCall(gridDim, blockDim, 0,
                    reinterpret_cast<etrtStream_t>(ctx->etStream));
  etrtSetupArgument(&args_struct, sizeof(args_struct), 0);
  etrtLaunch(nullptr, kernel_name.c_str());

  return ETDNN_STATUS_SUCCESS;
}

/*
 * y = α * W(x) + β * y
 *
 * See also: cpuConvolutionForward
 */
EXAPI etdnnStatus_t etdnnConvolutionForward(
    etdnnHandle_t handle, const void *alpha,
    const etdnnTensorDescriptor_t xDesc, const void *x,
    const etdnnFilterDescriptor_t wDesc, const void *w,
    const etdnnConvolutionDescriptor_t convDesc, etdnnConvolutionFwdAlgo_t algo,
    void *workSpace, size_t workSpaceSizeInBytes, const void *beta,
    const etdnnTensorDescriptor_t yDesc, void *y) {
  return etdnnBiasedConvolutionForward(handle, alpha, xDesc, x, wDesc, w,
                                       convDesc, NULL, algo, workSpace,
                                       workSpaceSizeInBytes, beta, yDesc, y);
}

EXAPI etdnnStatus_t etdnnSoftmaxForward(
    etdnnHandle_t handle, etdnnSoftmaxAlgorithm_t algorithm,
    etdnnSoftmaxMode_t mode, const void *alpha,
    const etdnnTensorDescriptor_t xDesc, const void *x, const void *beta,
    const etdnnTensorDescriptor_t yDesc, void *y) {
  EtDnnContext *ctx = (EtDnnContext *)handle;
  EtDnnTensorStruct *xt = (EtDnnTensorStruct *)xDesc;
  EtDnnTensorStruct *yt = (EtDnnTensorStruct *)yDesc;

  if (xt->dataType != yt->dataType || xt->dimensions != yt->dimensions) {
    return ETDNN_STATUS_BAD_PARAM;
  }

  for (int i = 0; i < xt->dimensions; i++) {
    if (xt->sizes[i] != yt->sizes[i]) {
      return ETDNN_STATUS_BAD_PARAM;
    }
  }

  {
    GetDev dev;

    assert(dev->isPtrAllocedDev(x));
    assert(dev->isPtrAllocedDev(y));
  }

  // Only 4D tensors are suported for now
  assert(xt->dimensions == 4);

  SoftmaxForward_KernelArgs_t args_struct = {xt->dataType,
                                             mode,
                                             xt->dimensions,
                                             xt->sizes[N_index],
                                             xt->sizes[C_index],
                                             xt->sizes[H_index],
                                             xt->sizes[W_index],
                                             xt->strides[N_index],
                                             xt->strides[C_index],
                                             xt->strides[H_index],
                                             xt->strides[W_index],
                                             yt->strides[N_index],
                                             yt->strides[C_index],
                                             yt->strides[H_index],
                                             yt->strides[W_index],
                                             x,
                                             y,
                                             {0},
                                             {0}};

  std::string kernel_name = "SoftmaxForward";

  if (mode == ETDNN_SOFTMAX_MODE_INSTANCE) {
    assert(args_struct.data_type == ETDNN_DATA_FLOAT);

    kernel_name = kernel_name + "_Instance";

    kernel_name = setArgsAlphaBetaAndFixKernelName(
        &args_struct, kernel_name, args_struct.data_type, alpha, beta);

    // All needed reduction performed in one Block.
    // For Block we request whole number of Shires, but no more than max
    // possible Block size. Each batch (n-dimension) is handled independently
    // from each other in separate Blocks.
    unsigned reduce_len = args_struct.c * args_struct.h * args_struct.w;
    dim3 gridDim(args_struct.n);
    dim3 blockDim(std::min<unsigned>(align_up(reduce_len, MINIONS_PER_SHIRE),
                                     MINIONS_COUNT));
    etrtConfigureCall(gridDim, blockDim, 0,
                      reinterpret_cast<etrtStream_t>(ctx->etStream));
    etrtSetupArgument(&args_struct, sizeof(args_struct), 0);
    etrtLaunch(nullptr, kernel_name.c_str());
  } else {
    // Only CUDNN_SOFTMAX_MODE_INSTANCE is supported for now
    abort();
  }

  return ETDNN_STATUS_SUCCESS;
}

EXAPI etdnnStatus_t etdnnTransformTensor(etdnnHandle_t handle,
                                         const void *alpha,
                                         const etdnnTensorDescriptor_t xDesc,
                                         const void *x, const void *beta,
                                         const etdnnTensorDescriptor_t yDesc,
                                         void *y) {
  EtDnnContext *ctx = (EtDnnContext *)handle;
  EtDnnTensorStruct *xt = (EtDnnTensorStruct *)xDesc;
  EtDnnTensorStruct *yt = (EtDnnTensorStruct *)yDesc;

  if (xt->dimensions != yt->dimensions) {
    abort();
  }

  if (xt->dimensions != 4) {
    abort();
  }

  for (int i = 0; i < xt->dimensions; i++) {
    if (yt->sizes[i] != xt->sizes[i]) {
      abort();
    }
  }

  if (xt->dataType != yt->dataType) {
    abort();
  }

  {
    GetDev dev;

    assert(dev->isPtrAllocedDev(x));
    assert(dev->isPtrAllocedDev(y));
  }

  TransformTensor_KernelArgs_t args_struct = {xt->dataType,
                                              xt->dimensions,
                                              xt->sizes[N_index],
                                              xt->sizes[C_index],
                                              xt->sizes[H_index],
                                              xt->sizes[W_index],
                                              xt->strides[N_index],
                                              xt->strides[C_index],
                                              xt->strides[H_index],
                                              xt->strides[W_index],
                                              yt->strides[N_index],
                                              yt->strides[C_index],
                                              yt->strides[H_index],
                                              yt->strides[W_index],
                                              x,
                                              y,
                                              {0},
                                              {0}};

  std::string kernel_name = setArgsAlphaBetaAndFixKernelName(
      &args_struct, "TransformTensor", args_struct.data_type, alpha, beta);

  dim3 gridDim(defaultGridDim1D(args_struct.n * args_struct.c * args_struct.h *
                                args_struct.w));
  dim3 blockDim(defaultBlockDim1D());
  etrtConfigureCall(gridDim, blockDim, 0,
                    reinterpret_cast<etrtStream_t>(ctx->etStream));
  etrtSetupArgument(&args_struct, sizeof(args_struct), 0);
  etrtLaunch(nullptr, kernel_name.c_str());

  return ETDNN_STATUS_SUCCESS;
}

EXAPI etdnnStatus_t etdnnSplat(etdnnHandle_t handle, const void *val,
                               const etdnnTensorDescriptor_t yDesc, void *y) {
  std::string kernel_name = "Splat";
  EtDnnContext *ctx = (EtDnnContext *)handle;
  EtDnnTensorStruct *yt = (EtDnnTensorStruct *)yDesc;

  {
    GetDev dev;
    assert(dev->isPtrAllocedDev(y));
    assert(yt->dimensions == 4);
  }

  Splat_KernelArgs_t kernel_args;
  kernel_args.dt = yt->dataType;
  if (yt->dataType == ETDNN_DATA_FLOAT) {
    memcpy(&kernel_args.data.f, val, sizeof(float));
    kernel_name += "_Float";
  } else if (yt->dataType == ETDNN_DATA_INT8) {
    memcpy(&kernel_args.data.i8, val, sizeof(int8_t));
    kernel_name += "_Int8";
  } else if (yt->dataType == ETDNN_DATA_INT32) {
    memcpy(&kernel_args.data.i32, val, sizeof(int32_t));
    kernel_name += "_Int32";
  } else if (yt->dataType == ETDNN_DATA_INT64) {
    memcpy(&kernel_args.data.i64, val, sizeof(int64_t));
    kernel_name += "_Int64";
  } else {
    abort();
  }
  int len = yt->sizes[N_index] * yt->sizes[C_index] * yt->sizes[H_index] *
            yt->sizes[W_index];
  kernel_args.y = y;
  kernel_args.count = len;

  dim3 gridDim(defaultGridDim1D(len));
  dim3 blockDim(defaultBlockDim1D());
  etrtConfigureCall(gridDim, blockDim, 0,
                    reinterpret_cast<etrtStream_t>(ctx->etStream));

  etrtSetupArgument(&kernel_args, sizeof(kernel_args), 0);
  etrtLaunch(nullptr, kernel_name.c_str());

  return ETDNN_STATUS_SUCCESS;
}

EXAPI etdnnStatus_t etdnnArithOp(etdnnHandle_t handle, etdnnArithOp_t op,
                                 const etdnnTensorDescriptor_t xDesc,
                                 const void *x,
                                 const etdnnTensorDescriptor_t yDesc,
                                 const void *y,
                                 const etdnnTensorDescriptor_t zDesc, void *z) {
  EtDnnContext *ctx = (EtDnnContext *)handle;
  EtDnnTensorStruct *xt = (EtDnnTensorStruct *)xDesc;
  EtDnnTensorStruct *yt = (EtDnnTensorStruct *)yDesc;
  EtDnnTensorStruct *zt = (EtDnnTensorStruct *)zDesc;

  std::string kernel_name = "";
  switch (op) {
  case ETDNN_TENSOR_ARITH_OP_ADD:
    kernel_name = kernel_name + "AddKernel";
    break;
  case ETDNN_TENSOR_ARITH_OP_SUB:
    kernel_name = kernel_name + "SubKernel";
    break;
  case ETDNN_TENSOR_ARITH_OP_MAX:
    kernel_name = kernel_name + "MaxKernel";
    break;
  case ETDNN_TENSOR_ARITH_OP_MIN:
    kernel_name = kernel_name + "MinKernel";
    break;
  case ETDNN_TENSOR_ARITH_OP_MUL:
    kernel_name = kernel_name + "MulKernel";
    break;
  case ETDNN_TENSOR_ARITH_OP_DIV:
    kernel_name = kernel_name + "DivKernel";
    break;
  case ETDNN_TENSOR_ARITH_OP_CMPLTE:
    kernel_name = kernel_name + "CmpLTEKernel";
    break;
  case ETDNN_TENSOR_ARITH_OP_CMPEQ:
    kernel_name = kernel_name + "CmpEQKernel";
    break;
  case ETDNN_TENSOR_ARITH_OP_POW:
    kernel_name = kernel_name + "PowKernel";
    break;
  default:
    abort();
    break;
  }

  kernel_name = fixKernelNameByDataType(kernel_name, zt->dataType);

  if (zt->dimensions != 4 || xt->dimensions != zt->dimensions ||
      yt->dimensions != zt->dimensions || xt->dataType != zt->dataType ||
      yt->dataType != zt->dataType) {
    abort();
  }

  for (int i = 0; i < xt->dimensions; i++) {
    if (yt->sizes[i] != zt->sizes[i] || xt->sizes[i] != zt->sizes[i]) {
      abort();
    }
  }

  {
    GetDev dev;
    assert(dev->isPtrAllocedDev(x));
    assert(dev->isPtrAllocedDev(y));
    assert(dev->isPtrAllocedDev(z));
  }

  int len = zt->sizes[N_index] * zt->sizes[C_index] * zt->sizes[H_index] *
            zt->sizes[W_index];
  dim3 gridDim(defaultGridDim1D(len));
  dim3 blockDim(defaultBlockDim1D());
  etrtConfigureCall(gridDim, blockDim, 0,
                    reinterpret_cast<etrtStream_t>(ctx->etStream));

  if (zt->dataType == ETDNN_DATA_INT8Q) {
    QuantizedArithOp_KernelArgs_t kernel_args = {
        op,        len,       (int8_t *)x, (int8_t *)y, (int8_t *)z, xt->scale,
        yt->scale, zt->scale, xt->offset,  yt->offset,  zt->offset};
    etrtSetupArgument(&kernel_args, sizeof(kernel_args), 0);
  } else {
    etrtSetupArgument(&len, 4, 0);
    etrtSetupArgument(&x, 8, 8);
    etrtSetupArgument(&y, 8, 16);
    etrtSetupArgument(&z, 8, 24);
  }
  etrtLaunch(nullptr, kernel_name.c_str());

  return ETDNN_STATUS_SUCCESS;
}

EXAPI etdnnStatus_t etdnnExtractTensor(etdnnHandle_t handle,
                                       const etdnnTensorDescriptor_t xDesc,
                                       const void *x,
                                       const etdnnTensorDescriptor_t yDesc,
                                       void *y, const size_t offsets[]) {
  EtDnnTensorStruct *xt = (EtDnnTensorStruct *)xDesc;
  EtDnnTensorStruct *yt = (EtDnnTensorStruct *)yDesc;

  EtDnnContext *ctx = (EtDnnContext *)handle;

  if (xt->dimensions != yt->dimensions || xt->dimensions != 4) {
    abort();
  }

  for (int i = 0; i < xt->dimensions; i++) {
    if (xt->sizes[i] < (int)(yt->sizes[i] + offsets[i])) {
      abort();
    }
  }

  if (xt->dataType != ETDNN_DATA_FLOAT) {
    abort();
  }
  if (xt->dataType != yt->dataType) {
    abort();
  }

  Insert_KernelArgs_t kernel_args;

  kernel_args.data_type = xt->dataType;
  kernel_args.dimensions = xt->dimensions;
  kernel_args.x = x;
  kernel_args.y = y;

  memcpy(kernel_args.sizes, yt->sizes, sizeof(kernel_args.sizes));

  kernel_args.x_offsets[0] = offsets[0];
  kernel_args.x_offsets[1] = offsets[1];
  kernel_args.x_offsets[2] = offsets[2];
  kernel_args.x_offsets[3] = offsets[3];
  memset(kernel_args.y_offsets, 0, sizeof(kernel_args.y_offsets));

  memcpy(kernel_args.x_strides, xt->strides, sizeof(kernel_args.x_strides));
  memcpy(kernel_args.y_strides, yt->strides, sizeof(kernel_args.y_strides));

  {
    GetDev dev;

    assert(dev->isPtrAllocedDev(x));
    assert(dev->isPtrAllocedDev(y));
  }

  std::string kernel_name = "Insert_Float";
  unsigned len = 1;
  for (int i = 0; i < kernel_args.dimensions; i++) {
    len *= kernel_args.sizes[i];
  }

  dim3 gridDim(defaultGridDim1D(len));
  dim3 blockDim(defaultBlockDim1D());
  etrtConfigureCall(gridDim, blockDim, 0,
                    reinterpret_cast<etrtStream_t>(ctx->etStream));
  etrtSetupArgument(&kernel_args, sizeof(kernel_args), 0);
  etrtLaunch(nullptr, kernel_name.c_str());

  return ETDNN_STATUS_SUCCESS;
}

EXAPI etdnnStatus_t etdnnInsertTensor(etdnnHandle_t handle,
                                      const etdnnTensorDescriptor_t xDesc,
                                      const void *x,
                                      const etdnnTensorDescriptor_t yDesc,
                                      void *y, const size_t offsets[]) {
  EtDnnTensorStruct *xt = (EtDnnTensorStruct *)xDesc;
  EtDnnTensorStruct *yt = (EtDnnTensorStruct *)yDesc;

  EtDnnContext *ctx = (EtDnnContext *)handle;

  if (xt->dimensions != yt->dimensions || xt->dimensions != 4) {
    abort();
  }

  for (int i = 0; i < xt->dimensions; i++) {
    if (yt->sizes[i] < (int)(xt->sizes[i] + offsets[i])) {
      abort();
    }
  }

  if (xt->dataType != ETDNN_DATA_FLOAT && xt->dataType != ETDNN_DATA_INT64) {
    abort();
  }
  if (xt->dataType != yt->dataType) {
    abort();
  }

  Insert_KernelArgs_t kernel_args;

  kernel_args.data_type = xt->dataType;
  kernel_args.dimensions = xt->dimensions;
  kernel_args.x = x;
  kernel_args.y = y;

  memcpy(kernel_args.sizes, yt->sizes, sizeof(kernel_args.sizes));

  memset(kernel_args.x_offsets, 0, sizeof(kernel_args.y_offsets));
  kernel_args.y_offsets[0] = offsets[0];
  kernel_args.y_offsets[1] = offsets[1];
  kernel_args.y_offsets[2] = offsets[2];
  kernel_args.y_offsets[3] = offsets[3];

  memcpy(kernel_args.x_strides, xt->strides, sizeof(kernel_args.x_strides));
  memcpy(kernel_args.y_strides, yt->strides, sizeof(kernel_args.y_strides));

  {
    GetDev dev;

    assert(dev->isPtrAllocedDev(x));
    assert(dev->isPtrAllocedDev(y));
  }

  std::string kernel_name = fixKernelNameByDataType("Insert", xt->dataType);
  unsigned len = 1;
  for (int i = 0; i < kernel_args.dimensions; i++) {
    len *= kernel_args.sizes[i];
  }

  dim3 gridDim(defaultGridDim1D(len));
  dim3 blockDim(defaultBlockDim1D());
  etrtConfigureCall(gridDim, blockDim, 0,
                    reinterpret_cast<etrtStream_t>(ctx->etStream));
  etrtSetupArgument(&kernel_args, sizeof(kernel_args), 0);
  etrtLaunch(nullptr, kernel_name.c_str());

  return ETDNN_STATUS_SUCCESS;
}

EXAPI etdnnStatus_t etdnnLocalResponseNormalization(
    etdnnHandle_t handle, const etdnnTensorDescriptor_t xDesc, const void *x,
    const etdnnTensorDescriptor_t yDesc, void *y,
    const etdnnTensorDescriptor_t scaleDesc, void *scale, long halfWindowSize,
    const void *k, const void *alpha, const void *beta) {
  EtDnnTensorStruct *xt = (EtDnnTensorStruct *)xDesc;
  EtDnnTensorStruct *yt = (EtDnnTensorStruct *)yDesc;
  EtDnnTensorStruct *scalet = (EtDnnTensorStruct *)scaleDesc;

  EtDnnContext *ctx = (EtDnnContext *)handle;

  if (xt->dimensions != scalet->dimensions ||
      yt->dimensions != scalet->dimensions || xt->dimensions != 4) {
    abort();
  }

  for (int i = 0; i < xt->dimensions; i++) {
    if (xt->sizes[i] != scalet->sizes[i] || yt->sizes[i] != scalet->sizes[i]) {
      abort();
    }
  }

  if (xt->dataType != ETDNN_DATA_FLOAT || xt->dataType != scalet->dataType ||
      yt->dataType != scalet->dataType) {
    abort();
  }

  LRN_KernelArgs_t args_struct = {xt->dataType,
                                  xt->dimensions,
                                  xt->sizes[N_index],
                                  xt->sizes[C_index],
                                  xt->sizes[H_index],
                                  xt->sizes[W_index],
                                  xt->strides[N_index],
                                  xt->strides[C_index],
                                  xt->strides[H_index],
                                  xt->strides[W_index],
                                  yt->strides[N_index],
                                  yt->strides[C_index],
                                  yt->strides[H_index],
                                  yt->strides[W_index],
                                  scalet->strides[N_index],
                                  scalet->strides[C_index],
                                  scalet->strides[H_index],
                                  scalet->strides[W_index],
                                  x,
                                  y,
                                  scale,
                                  halfWindowSize,
                                  {0},
                                  {0},
                                  {0}};

  args_struct.k.f = *(float *)k;
  args_struct.alpha.f = *(float *)alpha;
  args_struct.beta.f = *(float *)beta;

  {
    GetDev dev;

    assert(dev->isPtrAllocedDev(x));
    assert(dev->isPtrAllocedDev(y));
    assert(dev->isPtrAllocedDev(scale));
  }

  std::string kernel_name = "LRN_Float";
  unsigned len = args_struct.n * args_struct.c * args_struct.h * args_struct.w;

  dim3 gridDim(defaultGridDim1D(len));
  dim3 blockDim(defaultBlockDim1D());
  etrtConfigureCall(gridDim, blockDim, 0,
                    reinterpret_cast<etrtStream_t>(ctx->etStream));
  etrtSetupArgument(&args_struct, sizeof(args_struct), 0);
  etrtLaunch(nullptr, kernel_name.c_str());

  return ETDNN_STATUS_SUCCESS;
}

EXAPI etdnnStatus_t
etdnnSelect(etdnnHandle_t handle, const etdnnTensorDescriptor_t xDesc,
            const void *x, const etdnnTensorDescriptor_t yDesc, const void *y,
            const etdnnTensorDescriptor_t condDesc, const void *cond,
            const etdnnTensorDescriptor_t zDesc, void *z) {
  EtDnnContext *ctx = (EtDnnContext *)handle;
  EtDnnTensorStruct *xt = (EtDnnTensorStruct *)xDesc;
  EtDnnTensorStruct *yt = (EtDnnTensorStruct *)yDesc;
  EtDnnTensorStruct *zt = (EtDnnTensorStruct *)zDesc;
  EtDnnTensorStruct *condt = (EtDnnTensorStruct *)condDesc;

  if (zt->dimensions != 4 || xt->dimensions != zt->dimensions ||
      yt->dimensions != zt->dimensions || condt->dimensions != zt->dimensions ||
      xt->dataType != zt->dataType || yt->dataType != zt->dataType ||
      condt->dataType != zt->dataType) {
    abort();
  }

  for (int i = 0; i < xt->dimensions; i++) {
    if (yt->sizes[i] != zt->sizes[i] || xt->sizes[i] != zt->sizes[i] ||
        condt->sizes[i] != zt->sizes[i]) {
      abort();
    }
  }

  if (zt->dataType != ETDNN_DATA_FLOAT || xt->dataType != zt->dataType ||
      yt->dataType != zt->dataType || condt->dataType != zt->dataType) {
    abort();
  }

  {
    GetDev dev;
    assert(dev->isPtrAllocedDev(x));
    assert(dev->isPtrAllocedDev(y));
    assert(dev->isPtrAllocedDev(z));
    assert(dev->isPtrAllocedDev(cond));
  }

  std::string kernel_name = "SelectKernel_Float";
  int len = zt->sizes[N_index] * zt->sizes[C_index] * zt->sizes[H_index] *
            zt->sizes[W_index];
  dim3 gridDim(defaultGridDim1D(len));
  dim3 blockDim(defaultBlockDim1D());
  etrtConfigureCall(gridDim, blockDim, 0,
                    reinterpret_cast<etrtStream_t>(ctx->etStream));
  etrtSetupArgument(&len, 4, 0);
  etrtSetupArgument(&x, 8, 8);
  etrtSetupArgument(&y, 8, 16);
  etrtSetupArgument(&cond, 8, 24);
  etrtSetupArgument(&z, 8, 32);
  etrtLaunch(nullptr, kernel_name.c_str());

  return ETDNN_STATUS_SUCCESS;
}
