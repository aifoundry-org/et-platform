/* vim: set ts=8 sw=4 et sta cin cino=\:0s,l1,g0,N-s,E-s,i0,+2s,(0,W2s : */

#ifndef BEMU_FPU_H
#define BEMU_FPU_H

#include "fpu_types.h"

// ---------------------------------------------------------------------------
// RISCV and Esperanto additions to the softfloat floating-point operations.
// Similarly to softfloat these are added to the global namespace and then
// wrapped by functions with the same name in the fpu namespace.
// ---------------------------------------------------------------------------

float32_t f32_minimumNumber(float32_t, float32_t);
float32_t f32_maximumNumber(float32_t, float32_t);
float32_t f32_minNum(float32_t, float32_t);
float32_t f32_maxNum(float32_t, float32_t);

float11_t f32_to_f11(float32_t);
float32_t f11_to_f32(float11_t);

float10_t f32_to_f10(float32_t);
float32_t f10_to_f32(float10_t);

float32_t f32_frac(float32_t);

float32_t f32_sin2pi(float32_t);
float32_t f32_exp2(float32_t);
float32_t f32_log2(float32_t);
float32_t f32_rcp(float32_t);
float32_t f32_rsqrt(float32_t);

float32_t f1632_mulAdd2(float16_t, float16_t, float16_t, float16_t);
float32_t f1632_mulAdd3(float16_t, float16_t, float16_t, float16_t, float32_t);


// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

namespace fpu {

inline float32_t neg(float32_t x) {
    return float32_t { uint32_t(x.v ^ 0x80000000) };
}

inline float32_t daz(float32_t x) {
    float32_t z { (x.v & 0x7F800000) ? x.v : uint32_t(x.v & 0x80000000) };
    if (x.v != z.v) softfloat_raiseFlags(softfloat_flag_denormal);
    return z;
}

inline float16_t daz(float16_t x) {
    float16_t z { (x.v & 0x7C00) ? x.v : uint16_t(x.v & 0x8000) };
    if (x.v != z.v) softfloat_raiseFlags(softfloat_flag_denormal);
    return z;
}

inline float11_t daz(float11_t x) {
    float11_t z { (x.v & 0x7C0) ? x.v : uint16_t(0) };
    if (x.v != z.v) softfloat_raiseFlags(softfloat_flag_denormal);
    return z;
}

inline float10_t daz(float10_t x) {
    float10_t z { (x.v & 0x3E0) ? x.v : uint16_t(0) };
    if (x.v != z.v) softfloat_raiseFlags(softfloat_flag_denormal);
    return z;
}

inline float32_t ftz(float32_t x) {
    float32_t z { (x.v & 0x7F800000) ? x.v : uint32_t(x.v & 0x80000000) };
    if (x.v != z.v)
        softfloat_raiseFlags(softfloat_flag_underflow | softfloat_flag_inexact);
    return z;
}

inline float16_t ftz(float16_t x) {
    float16_t z { (x.v & 0x7C00) ? x.v : uint16_t(x.v & 0x8000) };
    if (x.v != z.v)
        softfloat_raiseFlags(softfloat_flag_underflow | softfloat_flag_inexact);
    return z;
}

} // namespace fpu


// ---------------------------------------------------------------------------
// RISC-V single-precision extension operations
// ---------------------------------------------------------------------------

namespace fpu {

inline float32_t f32_add(float32_t a, float32_t b) {
    return ftz( ::f32_add(daz(a), daz(b)) );
}

inline float32_t f32_sub(float32_t a, float32_t b) {
    return ftz( ::f32_sub(daz(a), daz(b)) );
}

inline float32_t f32_mul(float32_t a, float32_t b) {
    return ftz( ::f32_mul(daz(a), daz(b)) );
}

inline float32_t f32_div(float32_t a, float32_t b) {
    return ftz( ::f32_div(daz(a), daz(b)) );
}

inline float32_t f32_sqrt(float32_t a) {
    return ftz( ::f32_sqrt(daz(a)) );
}

inline float32_t f32_copySign(float32_t a, float32_t b) {
    return float32_t { (a.v & 0x7FFFFFFF) | (b.v & 0x80000000) };
}

inline float32_t f32_copySignNot(float32_t a, float32_t b) {
    return float32_t { (a.v & 0x7FFFFFFF) | ((~b.v) & 0x80000000) };
}

inline float32_t f32_copySignXor(float32_t a, float32_t b) {
    return float32_t { (a.v & 0x7FFFFFFF) | ((a.v ^ b.v) & 0x80000000) };
}

// NB: IEEE 754-201x compatible
inline float32_t f32_minimumNumber(float32_t a, float32_t b) {
    return ::f32_minimumNumber(daz(a), daz(b));
}

// NB: IEEE 754-201x compatible
inline float32_t f32_maximumNumber(float32_t a, float32_t b) {
    return ::f32_maximumNumber(daz(a), daz(b));
}

// NB: IEEE 754-2008 compatible
inline float32_t f32_minNum(float32_t a, float32_t b) {
    return ::f32_minNum(daz(a), daz(b));
}

// NB: IEEE 754-2008 compatible
inline float32_t f32_maxNum(float32_t a, float32_t b) {
    return ::f32_maxNum(daz(a), daz(b));
}

inline bool f32_eq(float32_t a, float32_t b) {
    return ::f32_eq(daz(a), daz(b));
}

inline bool f32_lt(float32_t a, float32_t b) {
    return ::f32_lt(daz(a), daz(b));
}

inline bool f32_le(float32_t a, float32_t b) {
    return ::f32_le(daz(a), daz(b));
}

inline int_fast32_t f32_to_i32(float32_t a) {
    return ::f32_to_i32(daz(a), softfloat_roundingMode, true);
}

inline uint_fast32_t f32_to_ui32(float32_t a) {
    return ::f32_to_ui32(daz(a), softfloat_roundingMode, true);
}

inline int_fast64_t f32_to_i64(float32_t a) {
    return ::f32_to_i64(daz(a), softfloat_roundingMode, true);
}

inline uint_fast64_t f32_to_ui64(float32_t a) {
    return ::f32_to_ui64(daz(a), softfloat_roundingMode, true);
}

inline uint_fast16_t f32_classify(float32_t a) {
    return ::f32_classify(a);
}

inline float32_t i32_to_f32(int32_t a) {
    return ::i32_to_f32(a);
}

inline float32_t ui32_to_f32(uint32_t a) {
    return ::ui32_to_f32(a);
}

inline float32_t i64_to_f32(int64_t a) {
    return ::i64_to_f32(a);
}

inline float32_t ui64_to_f32(uint64_t a) {
    return ::ui64_to_f32(a);
}

inline float32_t f32_mulAdd(float32_t a, float32_t b, float32_t c) {
    return ftz( ::f32_mulAdd(daz(a), daz(b), daz(c)) );
}

inline float32_t f32_mulSub(float32_t a, float32_t b, float32_t c) {
    return ftz( ::f32_mulAdd(daz(a), daz(b), neg(daz(c))) );
}

inline float32_t f32_negMulAdd(float32_t a, float32_t b, float32_t c) {
    return ftz( ::f32_mulAdd(neg(daz(a)), daz(b), neg(daz(c))) );
}

inline float32_t f32_negMulSub(float32_t a, float32_t b, float32_t c) {
    return ftz( ::f32_mulAdd(neg(daz(a)), daz(b), daz(c)) );
}

} // namespace fpu


// ---------------------------------------------------------------------------
// Esperanto single-precision extension operations
// ---------------------------------------------------------------------------

namespace fpu {

inline float16_t f32_to_f16(float32_t a) {
    return ftz( ::f32_to_f16(daz(a)) );
}

inline float11_t f32_to_f11(float32_t a) {
    return ::f32_to_f11(daz(a));
}

inline float10_t f32_to_f10(float32_t a) {
    return ::f32_to_f10(daz(a));
}

inline float32_t f16_to_f32(float16_t a) {
    return ::f16_to_f32(daz(a));
}

inline float32_t f11_to_f32(float11_t a) {
    return ::f11_to_f32(daz(a));
}

inline float32_t f10_to_f32(float10_t a) {
    return ::f10_to_f32(daz(a));
}

uint32_t f32_to_un24(float32_t);
uint16_t f32_to_un16(float32_t);
uint16_t f32_to_un10(float32_t);
uint8_t  f32_to_un8(float32_t);
uint8_t  f32_to_un2(float32_t);

uint32_t f32_to_sn24(float32_t);
uint16_t f32_to_sn16(float32_t);
uint8_t  f32_to_sn8(float32_t);

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

inline float32_t f32_frac(float32_t a) {
    return ::f32_frac(daz(a));
}

inline float32_t f32_roundToInt(float32_t a) {
    return ::f32_roundToInt(daz(a), softfloat_roundingMode, true);
}

inline float32_t f32_sin2pi(float32_t a) {
    return ftz( ::f32_sin2pi(daz(a)) );
}

inline float32_t f32_exp2(float32_t a) {
    return ftz( ::f32_exp2(daz(a)) );
}

inline float32_t f32_log2(float32_t a) {
    return ftz( ::f32_log2(daz(a)) );
}

inline float32_t f32_rcp(float32_t a) {
    return ftz( ::f32_rcp(daz(a)) );
}

inline float32_t f32_rsqrt(float32_t a) {
    return ftz( ::f32_rsqrt(daz(a)) );
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


inline float32_t f1632_mulAdd2(float16_t a1, float16_t b1,
                               float16_t a2, float16_t b2)
{
    return ftz( ::f1632_mulAdd2(daz(a1), daz(b1), daz(a2), daz(b2)) );
}


inline float32_t f1632_mulAdd3(float16_t a1, float16_t b1,
                               float16_t a2, float16_t b2, float32_t c)
{
    return ftz( ::f1632_mulAdd3(daz(a1), daz(b1), daz(a2), daz(b2), daz(c)) );
}


} // namespace fpu

#endif // BEMU_FPU_H
