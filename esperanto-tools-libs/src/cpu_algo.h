#ifndef ETTEE_CPU_ALGO_H
#define ETTEE_CPU_ALGO_H

#include <stdint.h>
#include <string>
#include <vector>
#include "../kernels/kernel_args.h"

void cpuSgemm_v2(Sgemm_v2_KernelArgs_t* args_p);
void cpuMatMulTensor(MatMulTensor_KernelArgs_t* args_p);
void cpuMatMulTensorInt8To32(MatMulTensorInt8To32_KernelArgs_t* args_p);
void cpuQuantizedMatMul(QuantizedMatMul_KernelArgs_t* args_p);
void cpuConvTailNHWC_Float(ConvTailNHWC_KernelArgs_t* args_p);
void cpuConvTailNHWC_Int8Q(ConvTailNHWC_KernelArgs_t* args_p);
void cpuConvolutionForward(ConvolutionForward_KernelArgs_t* args_p);
void cpuBatchNormalizationForwardInference(BatchNormalizationForwardInference_KernelArgs_t* args_p);
void cpuAddTensor(AddTensor_KernelArgs_t* args_p);
void cpuActivationForward(ActivationForward_KernelArgs_t* args_p);
void cpuPoolingForward(PoolingForward_KernelArgs_t* args_p);
void cpuSoftmaxForward( SoftmaxForward_KernelArgs_t * args_p );
void cpuTransformTensor(TransformTensor_KernelArgs_t* args_p);
void cpuBatchedAdd(BatchedAdd_KernelArgs_t* args_p);
void cpuQuantizedBatchedAdd(QuantizedBatchedAdd_KernelArgs_t* args_p);
void cpuInsert( Insert_KernelArgs_t * args_p);
void cpuGather( Gather_KernelArgs_t * args_p);
void cpuTopK( TopK_KernelArgs_t * args_p);
void cpuLRN( LRN_KernelArgs_t * args_p);
void cpuSplat(Splat_KernelArgs_t *args_p);
void cpuQuantizedArithOp(QuantizedArithOp_KernelArgs_t *args_p);

uintptr_t getBuiltinKernelPcByName(const std::string &kernel_name);
void cpuLaunch(const std::string &kernel_name, uintptr_t kernel_pc, const std::vector<uint8_t> &args_buff);

#endif  // ETTEE_CPU_ALGO_H

