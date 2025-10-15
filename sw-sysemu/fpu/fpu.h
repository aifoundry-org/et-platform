/*-------------------------------------------------------------------------
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------*/

#ifndef BEMU_FPU_H
#define BEMU_FPU_H

#include "softfloat/platform.h"
#include "softfloat/softfloat.h"
#include "fpu_types.h"

// ---------------------------------------------------------------------------
// Esperanto additions to the softfloat floating-point operations.  Similarly
// to softfloat these are added to the global namespace and then wrapped by
// functions with the same name in the fpu namespace.
// ---------------------------------------------------------------------------

extern "C" float32_t f32_copySign(float32_t, float32_t);
extern "C" float32_t f32_copySignNot(float32_t, float32_t);
extern "C" float32_t f32_copySignXor(float32_t, float32_t);

extern "C" float32_t f32_maxNum(float32_t, float32_t);
extern "C" float32_t f32_minNum(float32_t, float32_t);

extern "C" float32_t f32_maximumNumber(float32_t, float32_t);
extern "C" float32_t f32_minimumNumber(float32_t, float32_t);

float11_t f32_to_f11(float32_t);
float32_t f11_to_f32(float11_t);

float10_t f32_to_f10(float32_t);
float32_t f10_to_f32(float10_t);

float32_t f32_frac(float32_t);

float32_t f32_sin2pi(float32_t);
float32_t f32_exp2(float32_t);
extern "C" float32_t f32_log2(float32_t);
float32_t f32_rcp(float32_t);
float32_t f32_rsqrt(float32_t);

extern "C" float32_t f32_mulSub(float32_t, float32_t, float32_t);
extern "C" float32_t f32_subMulAdd(float32_t, float32_t, float32_t);
extern "C" float32_t f32_subMulSub(float32_t, float32_t, float32_t);

float32_t f1632_mulAdd2(float16_t, float16_t, float16_t, float16_t);
float32_t f1632_mulAdd3(float16_t, float16_t, float16_t, float16_t, float32_t);


// ---------------------------------------------------------------------------
// RISC-V single-precision extension operations
// ---------------------------------------------------------------------------

namespace fpu {


using ::f32_add;
using ::f32_classify;
using ::f32_copySign;
using ::f32_copySignNot;
using ::f32_copySignXor;
using ::f32_div;
using ::f32_eq;
using ::f32_le;
using ::f32_lt;
using ::f32_maxNum;
using ::f32_maximumNumber;
using ::f32_minNum;
using ::f32_minimumNumber;
using ::f32_mul;
using ::f32_mulAdd;
using ::f32_mulSub;
using ::f32_sqrt;
using ::f32_sub;
using ::f32_subMulAdd;
using ::f32_subMulSub;
using ::i32_to_f32;
using ::i64_to_f32;
using ::ui32_to_f32;
using ::ui64_to_f32;


inline int_fast32_t f32_to_i32(float32_t a) {
    return ::f32_to_i32(a, softfloat_roundingMode, true);
}


inline uint_fast32_t f32_to_ui32(float32_t a) {
    return ::f32_to_ui32(a, softfloat_roundingMode, true);
}


inline int_fast64_t f32_to_i64(float32_t a) {
    return ::f32_to_i64(a, softfloat_roundingMode, true);
}


inline uint_fast64_t f32_to_ui64(float32_t a) {
    return ::f32_to_ui64(a, softfloat_roundingMode, true);
}


} // namespace fpu


// ---------------------------------------------------------------------------
// Esperanto single-precision extension operations
// ---------------------------------------------------------------------------

namespace fpu {

using ::f10_to_f32;
using ::f11_to_f32;
using ::f16_to_f32;
using ::f32_exp2;
using ::f32_frac;
using ::f32_log2;
using ::f32_rcp;
using ::f32_rsqrt;
using ::f32_sin2pi;
using ::f32_to_f10;
using ::f32_to_f11;
using ::f32_to_f16;

uint_fast32_t f32_to_un24(float32_t);
uint_fast16_t f32_to_un16(float32_t);
uint_fast16_t f32_to_un10(float32_t);
uint_fast8_t  f32_to_un8(float32_t);
uint_fast8_t  f32_to_un2(float32_t);

uint_fast32_t f32_to_sn24(float32_t);
uint_fast16_t f32_to_sn16(float32_t);
uint_fast8_t  f32_to_sn8(float32_t);

float32_t un24_to_f32(uint32_t);
float32_t un16_to_f32(uint16_t);
float32_t un10_to_f32(uint16_t);
float32_t un8_to_f32(uint8_t);
float32_t un2_to_f32(uint8_t);

//float32_t sn24_to_f32(uint32_t);
float32_t sn16_to_f32(uint16_t);
float32_t sn10_to_f32(uint16_t);
float32_t sn8_to_f32(uint8_t);
float32_t sn2_to_f32(uint8_t);

inline float32_t f32_roundToInt(float32_t a) {
    return ::f32_roundToInt(a, softfloat_roundingMode, true);
}

float32_t f32_cubeFaceIdx(uint8_t, float32_t);
float32_t f32_cubeFaceSignS(uint8_t, float32_t);
float32_t f32_cubeFaceSignT(uint8_t, float32_t);

float32_t fxp1516_to_f32(int32_t);
int32_t f32_to_fxp1714(float32_t);

int32_t fxp1714_rcpStep(int32_t, int32_t);

} // namespace fpu


// ---------------------------------------------------------------------------
// Esperanto tensor extension operations
// ---------------------------------------------------------------------------

namespace fpu {

using ::f1632_mulAdd2;
using ::f1632_mulAdd3;

} // namespace fpu

#endif // BEMU_FPU_H
