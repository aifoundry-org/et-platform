#ifndef _ETLIBDEVICE_H_
#define _ETLIBDEVICE_H_

#include "../kernels/sys_inc.h"

typedef struct {
  unsigned grid_size_x;  // CUDA gridDim.x
  unsigned grid_size_y;  // CUDA gridDim.y
  unsigned grid_size_z;  // CUDA gridDim.z
  unsigned block_size_x; // CUDA blockDim.x
  unsigned block_size_y; // CUDA blockDim.y
  unsigned block_size_z; // CUDA blockDim.z
} KernelRuntimeState_t;

typedef struct {
  KernelRuntimeState_t state;
  uint64_t kernel_pc;
  uint64_t args_size;
  char args[0];
} LaunchParams_t;

typedef struct {
  unsigned block_id_x; // CUDA blockIdx.x
  unsigned block_id_y; // CUDA blockIdx.y
  unsigned block_id_z; // CUDA blockIdx.z
  unsigned local_id_x; // CUDA threadIdx.x
  unsigned local_id_y; // CUDA threadIdx.y
  unsigned local_id_z; // CUDA threadIdx.z

  unsigned shires_per_block;
  unsigned this_shire_active_minions;
} KernelWorkItemState_t;

#ifndef INCLUDE_FOR_HOST

typedef void (*KernelWorkItemFunction_t)(void *args,
                                         KernelRuntimeState_t *state_p,
                                         KernelWorkItemState_t *wi_state_p);

extern "C" void KernelDispatchFunction(void *args,
                                       KernelRuntimeState_t *state_p,
                                       KernelWorkItemFunction_t kernel_wi_fun);

extern "C" char *__et_get_smem_vars_storage(KernelRuntimeState_t *rt_p,
                                            KernelWorkItemState_t *wi_p);
extern "C" void llvm_nvvm_barrier0(KernelRuntimeState_t *rt_p,
                                   KernelWorkItemState_t *wi_p);

// Auxiliary functions
AUX_FUN_ATTRS float device_exp2f(float a) {
  float res;
  asm volatile("fexp.ps %0, %1\n" : "=f"(res) : "f"(a) :);
  return res;
}

AUX_FUN_ATTRS float device_log2f(float a) {
  float res;
  asm volatile("flog.ps %0, %1\n" : "=f"(res) : "f"(a) :);
  return res;
}

AUX_FUN_ATTRS float device_rcpf(float a) {
  float res;
  asm volatile("frcp.ps %0, %1\n" : "=f"(res) : "f"(a) :);
  return res;
}

AUX_FUN_ATTRS float device_maxf(float a, float b) {
  float res;
  asm volatile("fmax.ps %0, %1, %2\n" : "=f"(res) : "f"(a), "f"(b) :);
  return res;
}

AUX_FUN_ATTRS float device_minf(float a, float b) {
  float res;
  asm volatile("fmin.ps %0, %1, %2\n" : "=f"(res) : "f"(a), "f"(b) :);
  return res;
}

AUX_FUN_ATTRS float device_sqrtf(float a) {
  float res;
  asm volatile("fsqrt.ps %0, %1\n" : "=f"(res) : "f"(a) :);
  return res;
}

// like libc roundf: round x to the nearest integer, but round halfway cases
// away from zero
AUX_FUN_ATTRS float device_roundf(float a) {
  float res;
  asm volatile("fround.ps %0, %1, rmm\n" : "=f"(res) : "f"(a) :);
  return res;
}

static constexpr float CONST_log2e_f = 1.44269504089f;

AUX_FUN_ATTRS float device_expf(float a) {
  return device_exp2f(CONST_log2e_f * a);
}

AUX_FUN_ATTRS float device_powf(float a, float b) {
  return device_exp2f(device_log2f(a) * b);
}

AUX_FUN_ATTRS float device_logf(float a) {
  return device_log2f(a) / CONST_log2e_f;
}

AUX_FUN_ATTRS int device_min(int a, int b) { return a < b ? a : b; }

AUX_FUN_ATTRS int device_max(int a, int b) { return a > b ? a : b; }

#endif /* !INCLUDE_FOR_HOST */

#endif /* _ETLIBDEVICE_H_ */
