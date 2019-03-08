#ifndef ETDNN_H
#define ETDNN_H

#include <stddef.h>
#include <stdint.h>
#include "etdnn-bin.h"
#include "etrt.h"
#include "et-misc.h"

typedef enum
{
    ETDNN_TENSOR_ARITH_OP_ADD = 0,
    ETDNN_TENSOR_ARITH_OP_SUB = 1,
    ETDNN_TENSOR_ARITH_OP_MUL = 2,
    ETDNN_TENSOR_ARITH_OP_DIV = 3,
    ETDNN_TENSOR_ARITH_OP_MAX = 4,
    ETDNN_TENSOR_ARITH_OP_MIN = 5,
    ETDNN_TENSOR_ARITH_OP_CMPLTE = 6,
    ETDNN_TENSOR_ARITH_OP_CMPEQ = 7,
    ETDNN_TENSOR_ARITH_OP_POW = 8,
} etdnnArithOp_t;

EXAPI
size_t etdnnGetVersion();
EXAPI
etdnnStatus_t etdnnCreate(etdnnHandle_t *handle);
EXAPI
etdnnStatus_t etdnnDestroy(etdnnHandle_t handle);
EXAPI
etdnnStatus_t etdnnSetStream(etdnnHandle_t handle, etrtStream_t streamId);
EXAPI
etdnnStatus_t etdnnGetStream(etdnnHandle_t handle, etrtStream_t *streamId);
EXAPI
etdnnStatus_t etdnnCreateTensorDescriptor( etdnnTensorDescriptor_t *tensorDesc);
EXAPI
etdnnStatus_t etdnnDestroyTensorDescriptor( etdnnTensorDescriptor_t tensorDesc);
EXAPI
etdnnStatus_t etdnnSetTensor4dDescriptorEx( etdnnTensorDescriptor_t     tensorDesc,
                                            etdnnDataType_t             dataType,
                                            int                         n,
                                            int                         c,
                                            int                         h,
                                            int                         w,
                                            int                         nStride,
                                            int                         cStride,
                                            int                         hStride,
                                            int                         wStride);

EXAPI
etdnnStatus_t etdnnSetTensor4dDescriptor( etdnnTensorDescriptor_t     tensorDesc,
                                          etdnnTensorFormat_t         format,
                                          etdnnDataType_t             dataType,
                                          int                         n,
                                          int                         c,
                                          int                         h,
                                          int                         w);

EXAPI
etdnnStatus_t etdnnSetTensorNdDescriptor( etdnnTensorDescriptor_t tensorDesc,
                                          etdnnDataType_t         dataType,
                                          int                     nbDims,
                                          const int               *dimA,
                                          const int               *strideA);

EXAPI
etdnnStatus_t etdnnSetTensorNdDescriptorEx( etdnnTensorDescriptor_t tensorDesc,
                                            etdnnTensorFormat_t     format,
                                            etdnnDataType_t         dataType,
                                            int                     nbDims,
                                            const int               *dimA);

EXAPI etdnnStatus_t
etdnnSetTensorQuantizationParams( etdnnTensorDescriptor_t tensorDesc,
                                  float                   scale,
                                  int32_t                 offset);

EXAPI
etdnnStatus_t etdnnCreateFilterDescriptor( etdnnFilterDescriptor_t *filterDesc);
EXAPI
etdnnStatus_t etdnnDestroyFilterDescriptor( etdnnFilterDescriptor_t filterDesc);
EXAPI
etdnnStatus_t etdnnSetFilter4dDescriptor( etdnnFilterDescriptor_t    filterDesc,
                                          etdnnDataType_t            dataType,
                                          etdnnTensorFormat_t        format,
                                          int                        k,
                                          int                        c,
                                          int                        h,
                                          int                        w);
EXAPI etdnnStatus_t
etdnnSetFilterQuantizationParams( etdnnFilterDescriptor_t filterDesc,
                                  float                   scale,
                                  int32_t                 offset);

EXAPI
etdnnStatus_t etdnnCreateConvolutionDescriptor( etdnnConvolutionDescriptor_t *convDesc);
EXAPI
etdnnStatus_t etdnnDestroyConvolutionDescriptor( etdnnConvolutionDescriptor_t convDesc);
EXAPI
etdnnStatus_t etdnnSetConvolution2dDescriptor( etdnnConvolutionDescriptor_t    convDesc,
                                               int                             pad_h,
                                               int                             pad_w,
                                               int                             u,
                                               int                             v,
                                               int                             dilation_h,
                                               int                             dilation_w,
                                               etdnnConvolutionMode_t          mode,
                                               etdnnDataType_t                 computeType);
EXAPI
etdnnStatus_t etdnnSetConvolutionGroupCount( etdnnConvolutionDescriptor_t convDesc,
                                             int groupCount);
EXAPI etdnnStatus_t
etdnnSetConvolutionBiasQuantizationParams( etdnnConvolutionDescriptor_t convDesc,
                                           float                        scale,
                                           int32_t                      offset);

EXAPI
etdnnStatus_t etdnnCreateActivationDescriptor( etdnnActivationDescriptor_t *activationDesc);
EXAPI
etdnnStatus_t etdnnDestroyActivationDescriptor( etdnnActivationDescriptor_t activationDesc);
EXAPI
etdnnStatus_t etdnnSetActivationDescriptor( etdnnActivationDescriptor_t         activationDesc,
                                            etdnnActivationMode_t               mode,
                                            etdnnNanPropagation_t               reluNanOpt,
                                            double                              coef);

EXAPI
etdnnStatus_t etdnnCreatePoolingDescriptor( etdnnPoolingDescriptor_t   *poolingDesc);
EXAPI
etdnnStatus_t etdnnDestroyPoolingDescriptor( etdnnPoolingDescriptor_t poolingDesc);
EXAPI
etdnnStatus_t etdnnSetPooling2dDescriptor( etdnnPoolingDescriptor_t    poolingDesc,
                                           etdnnPoolingMode_t          mode,
                                           etdnnNanPropagation_t       maxpoolingNanOpt,
                                           int                         windowHeight,
                                           int                         windowWidth,
                                           int                         verticalPadding,
                                           int                         horizontalPadding,
                                           int                         verticalStride,
                                           int                         horizontalStride);

EXAPI
etdnnStatus_t etdnnActivationForward( etdnnHandle_t handle,
                                      etdnnActivationDescriptor_t     activationDesc,
                                      const void                     *alpha,
                                      const etdnnTensorDescriptor_t   xDesc,
                                      const void                     *x,
                                      const void                     *beta,
                                      const etdnnTensorDescriptor_t   yDesc,
                                      void                           *y);

EXAPI
etdnnStatus_t etdnnActivationLinear( etdnnHandle_t handle,
                                     etdnnActivationMode_t           mode,
                                     const etdnnTensorDescriptor_t   xDesc,
                                     const void                     *x,
                                     const etdnnTensorDescriptor_t   yDesc,
                                     void                           *y);

EXAPI
etdnnStatus_t etdnnPoolingForward( etdnnHandle_t                    handle,
                                   const etdnnPoolingDescriptor_t   poolingDesc,
                                   const void                      *alpha,
                                   const etdnnTensorDescriptor_t    xDesc,
                                   const void                      *x,
                                   const void                      *beta,
                                   const etdnnTensorDescriptor_t    yDesc,
                                   void                            *y);

EXAPI
etdnnStatus_t etdnnAddTensor( etdnnHandle_t                    handle,
                              const void                       *alpha,
                              const etdnnTensorDescriptor_t     aDesc,
                              const void                       *A,
                              const void                       *beta,
                              const etdnnTensorDescriptor_t     cDesc,
                              void                             *C);

EXAPI
etdnnStatus_t etdnnDeriveBNTensorDescriptor( etdnnTensorDescriptor_t         derivedBnDesc,
                                             const etdnnTensorDescriptor_t   xDesc,
                                             etdnnBatchNormMode_t            mode);

EXAPI
etdnnStatus_t etdnnBatchNormalizationForwardInference( etdnnHandle_t                    handle,
                                                       etdnnBatchNormMode_t             mode,
                                                       const void                      *alpha,
                                                       const void                      *beta,
                                                       const etdnnTensorDescriptor_t    xDesc,
                                                       const void                      *x,
                                                       const etdnnTensorDescriptor_t    yDesc,
                                                       void                            *y,
                                                       const etdnnTensorDescriptor_t    bnScaleBiasMeanVarDesc,
                                                       const void                      *bnScale,
                                                       const void                      *bnBias,
                                                       const void                      *estimatedMean,
                                                       const void                      *estimatedVariance,
                                                       double                           epsilon);

EXAPI
etdnnStatus_t etdnnGetConvolutionForwardAlgorithm( etdnnHandle_t                      handle,
                                                   const etdnnTensorDescriptor_t      xDesc,
                                                   const etdnnFilterDescriptor_t      wDesc,
                                                   const etdnnConvolutionDescriptor_t convDesc,
                                                   const etdnnTensorDescriptor_t      yDesc,
                                                   etdnnConvolutionFwdPreference_t    preference,
                                                   size_t                             memoryLimitInBytes,
                                                   etdnnConvolutionFwdAlgo_t         *algo);
EXAPI
etdnnStatus_t etdnnGetConvolutionForwardWorkspaceSize( etdnnHandle_t                           handle,
                                                       const   etdnnTensorDescriptor_t         xDesc,
                                                       const   etdnnFilterDescriptor_t         wDesc,
                                                       const   etdnnConvolutionDescriptor_t    convDesc,
                                                       const   etdnnTensorDescriptor_t         yDesc,
                                                       etdnnConvolutionFwdAlgo_t               algo,
                                                       size_t                                 *sizeInBytes);
EXAPI
etdnnStatus_t etdnnConvolutionForward( etdnnHandle_t                       handle,
                                       const void                         *alpha,
                                       const etdnnTensorDescriptor_t       xDesc,
                                       const void                         *x,
                                       const etdnnFilterDescriptor_t       wDesc,
                                       const void                         *w,
                                       const etdnnConvolutionDescriptor_t  convDesc,
                                       etdnnConvolutionFwdAlgo_t           algo,
                                       void                               *workSpace,
                                       size_t                              workSpaceSizeInBytes,
                                       const void                         *beta,
                                       const etdnnTensorDescriptor_t       yDesc,
                                       void                               *y);

EXAPI
etdnnStatus_t etdnnBiasedConvolutionForward( etdnnHandle_t                       handle,
                                             const void                         *alpha,
                                             const etdnnTensorDescriptor_t       xDesc,
                                             const void                         *x,
                                             const etdnnFilterDescriptor_t       wDesc,
                                             const void                         *w,
                                             const etdnnConvolutionDescriptor_t  convDesc,
                                             const void                         *bias,
                                             etdnnConvolutionFwdAlgo_t           algo,
                                             void                               *workSpace,
                                             size_t                              workSpaceSizeInBytes,
                                             const void                         *beta,
                                             const etdnnTensorDescriptor_t       yDesc,
                                             void                               *y);

EXAPI
etdnnStatus_t etdnnSoftmaxForward( etdnnHandle_t                    handle,
                                   etdnnSoftmaxAlgorithm_t          algorithm,
                                   etdnnSoftmaxMode_t               mode,
                                   const void                      *alpha,
                                   const etdnnTensorDescriptor_t    xDesc,
                                   const void                      *x,
                                   const void                      *beta,
                                   const etdnnTensorDescriptor_t    yDesc,
                                   void                            *y);

EXAPI
etdnnStatus_t etdnnTransformTensor( etdnnHandle_t                  handle,
                                    const void                    *alpha,
                                    const etdnnTensorDescriptor_t  xDesc,
                                    const void                    *x,
                                    const void                    *beta,
                                    const etdnnTensorDescriptor_t  yDesc,
                                    void                          *y);

EXAPI
etdnnStatus_t etdnnSplat( etdnnHandle_t                    handle,
                          const void                       *val,
                          const etdnnTensorDescriptor_t    yDesc,
                          void                             *y);

EXAPI
etdnnStatus_t etdnnArithOp( etdnnHandle_t                    handle,
                            etdnnArithOp_t                   op,
                            const etdnnTensorDescriptor_t    xDesc,
                            const void *                     x,
                            const etdnnTensorDescriptor_t    yDesc,
                            const void *                     y,
                            const etdnnTensorDescriptor_t    zDesc,
                            void *                           z);

EXAPI
etdnnStatus_t etdnnExtractTensor( etdnnHandle_t                    handle,
                                  const etdnnTensorDescriptor_t    xDesc,
                                  const void *                     x,
                                  const etdnnTensorDescriptor_t    yDesc,
                                  void *                           y,
                                  const size_t offsets[]);

EXAPI
etdnnStatus_t etdnnInsertTensor( etdnnHandle_t                    handle,
                                 const etdnnTensorDescriptor_t    xDesc,
                                 const void *                     x,
                                 const etdnnTensorDescriptor_t    yDesc,
                                 void *                           y,
                                 const size_t offsets[]);

EXAPI
etdnnStatus_t etdnnLocalResponseNormalization( etdnnHandle_t                    handle,
                                               const etdnnTensorDescriptor_t    xDesc,
                                               const void *                     x,
                                               const etdnnTensorDescriptor_t    yDesc,
                                               void *                           y,
                                               const etdnnTensorDescriptor_t    scaleDesc,
                                               void *                           scale,
                                               long                             halfWindowSize,
                                               const void *                     k,
                                               const void *                     alpha,
                                               const void *                     beta);

EXAPI
etdnnStatus_t etdnnSelect( etdnnHandle_t                    handle,
                           const etdnnTensorDescriptor_t    xDesc,
                           const void *                     x,
                           const etdnnTensorDescriptor_t    yDesc,
                           const void *                     y,
                           const etdnnTensorDescriptor_t    condDesc,
                           const void *                     cond,
                           const etdnnTensorDescriptor_t    zDesc,
                           void *                           z);

#endif  // ETDNN_H

