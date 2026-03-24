/*-------------------------------------------------------------------------
 * Copyright (c) 2025 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------
 */

#include <etsoc/common/utils.h>
#include <etsoc/isa/hart.h>
#include <algorithm>
#include <cstdint>

#include "entryPoint.h"
#include "saxpy_kernel_arguments.h"
#include <etsoc/isa/tensors.h>

int entryPoint_0(KernelArguments* args);
DECLARE_KERNEL_ENTRY_POINTS(entryPoint_0, nullptr);

//clang-format off
static inline __attribute__((always_inline))
void saxpy_vector(const size_t begin, const size_t end, const float alpha,
  const float* const x,  const float* const y, float* const w)
{
    constexpr int vlen = 8;
    float alphaVector;
    constexpr uint32_t mask = 0xff;
#ifndef __clang__
    mask_set(0, mask);
#endif
    auto i = begin;
     __asm__ __volatile__("fbcx.ps %[alphaVector], %[alpha]\n"
                          : [ alphaVector ] "=&f"(alphaVector)
                          : [ alpha ] "r"(alpha)
                        #ifdef __clang__
                          , [ mask ] "M"(mask) 
                        #endif
                          :);
    for (; i < end - (vlen - 1); i += vlen) {
        float xValue;
        float yValue;
        const float *xv = &x[i]; 
        const float *yv = &y[i];
        const float *wv = &w[i];
        // load x and y
        __asm__ __volatile__ ("flw.ps %[xValue], 0(%[xv])\n"
                              "flw.ps %[yValue], 0(%[yv])\n"
                              : [xValue] "=&f"(xValue),
                                [yValue] "=&f"(yValue)
                              : [xv] "r" (xv), [yv] "r" (yv)

                          #ifdef __clang__
                              , [ mask ] "M"(mask) 
                          #endif
                              : );

        // w[i] = a * x[i] + y[i] 
        __asm__ __volatile__(
                            // multipy datavalues by weight and accumulate to prev value
                            "fmadd.ps %[yValue], %[xValue], %[alphaVector], %[yValue]\n"
                            : [yValue] "+&f"(yValue)
                            : [ xValue ] "f"(xValue), [ alphaVector ] "f"(alphaVector)
                          #ifdef __clang__
                            , [ mask ] "M"(mask) 
                          #endif
                            :);

        // store w[i]
        __asm__ __volatile__ ("fsw.ps %[yValue], 0(%[wv])\n"
                              :         
                              : [wv] "r" (wv), [yValue] "f"(yValue)
                          #ifdef __clang__
                              , [ mask ] "M"(mask)
                          #endif 
                              : );
    }

    for (; i < end; ++i) {
      w[i] = alpha * x[i] + y[i];
    }
    // epilogue
}
//clang-format on

int entryPoint_0(KernelArguments* vectors) {

  auto minionId = get_relative_thread_id();
  size_t numWorkers = SOC_MINIONS_PER_SHIRE; // just 1 shire (32 minions).

  size_t elemsPerWorker = (vectors->numElements + numWorkers - 1) / numWorkers;
  if (elemsPerWorker % 16) {
    elemsPerWorker += 16 - (elemsPerWorker % 16);
  }
  size_t begin = elemsPerWorker * minionId;
  size_t end = std::min(elemsPerWorker * (minionId + 1), vectors->numElements);

  if (begin > (vectors->numElements - 1)) {
    return 0;
  }

#ifdef SAXPY_VECTOR
    saxpy_vector(begin, end, vectors->a, vectors->x, vectors->y, vectors->y);
#else
     for (size_t i = begin; i < end; ++i) {
       vectors->y[i] = vectors->a * vectors->x[i] + vectors->y[i];
     }
#endif

  return 0;
}
