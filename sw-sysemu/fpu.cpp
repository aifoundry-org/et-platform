#include "fpu.h"
#include "fpu_casts.h"
#include "cvt.h"
#include "ttrans.h"
#include "softfloat/softfloat.h"
#include "softfloat/internals.h"
#include "softfloat/specialize.h"

#include <cmath> // FIXME: remove this when we fix f32_frac()

// Set to convert input denormals as zero
#ifndef FPU_DAZ
#define FPU_DAZ 1
#endif


// Set to convert output denormals to zero
#ifndef FPU_FTZ
#define FPU_FTZ 1
#endif


// -----------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------

namespace fpu {

    static inline float32_t neg(float32_t x)
    {
        return F32(UI32(x) ^ 0x80000000);
    }


#if defined(FPU_DAZ) || defined(FPU_FTZ)
    static inline uint_fast32_t subnormalToZeroF32UI(uint_fast32_t x)
    {
        return ((x & 0x7F800000) == 0) ? (x & 0x80000000) : x;
    }

    static inline uint_fast16_t subnormalToZeroF16UI(uint_fast16_t x)
    {
        return ((x & 0x7C00) == 0) ? (x & 0x8000) : x;
    }

    static inline uint_fast16_t subnormalToZeroF11UI(uint_fast16_t x)
    {
        return ((x & 0x7C0) == 0) ? 0 : x;
    }

    static inline uint_fast16_t subnormalToZeroF10UI(uint_fast16_t x)
    {
        return ((x & 0x3E0) == 0) ? 0 : x;
    }
#endif


    static inline float32_t daz(float32_t x)
    {
#ifdef FPU_DAZ
        return F32(subnormalToZeroF32UI(UI32(x)));
#else
        return x;
#endif
    }


    static inline float16_t daz(float16_t x)
    {
#ifdef FPU_DAZ
        return F16(subnormalToZeroF16UI(UI16(x)));
#else
        return x;
#endif
    }


    static inline float11_t daz(float11_t x)
    {
#ifdef FPU_DAZ
        return F11(subnormalToZeroF11UI(UI16(x)));
#else
        return x;
#endif
    }


    static inline float10_t daz(float10_t x)
    {
#ifdef FPU_DAZ
        return F10(subnormalToZeroF10UI(UI16(x)));
#else
        return x;
#endif
    }


    static inline float32_t ftz(float32_t x)
    {
#ifdef FPU_FTZ
        return F32(subnormalToZeroF32UI(UI32(x)));
#else
        return x;
#endif
    }

} // namespace fpu


// -----------------------------------------------------------------------
// Extensions to softfloat
// -----------------------------------------------------------------------

static uint_fast32_t
    softfloat_propagateF32UI( uint_fast32_t uiA, uint_fast32_t uiB )
{
    if ( softfloat_isSigNaNF32UI( uiA ) || softfloat_isSigNaNF32UI( uiB ) ) {
        softfloat_raiseFlags( softfloat_flag_invalid );
    }
    return isNaNF32UI( uiA )
        ? ( isNaNF32UI( uiB ) ? defaultNaNF32UI : uiB )
        : uiA;
}


static float32_t f32_minimumNumber( float32_t a, float32_t b )
{
    union ui32_f32 uA;
    uint_fast32_t uiA;
    union ui32_f32 uB;
    uint_fast32_t uiB;
    bool signA, signB;

    uA.f = a;
    uiA = uA.ui;
    uB.f = b;
    uiB = uB.ui;
    if ( isNaNF32UI( uiA ) || isNaNF32UI( uiB ) ) {
        union ui32_f32 uZ;
        uint_fast32_t uiZ;
        uiZ = softfloat_propagateF32UI( uiA, uiB );
        uZ.ui = uiZ;
        return uZ.f;
    }
    signA = signF32UI( uiA );
    signB = signF32UI( uiB );

    return ( signA != signB )
        ? ( signA ? a : b )
        : ( ((uiA != uiB) && (signA ^ (uiA < uiB))) ? a : b );
}


static float32_t f32_maximumNumber( float32_t a, float32_t b )
{
    union ui32_f32 uA;
    uint_fast32_t uiA;
    union ui32_f32 uB;
    uint_fast32_t uiB;
    bool signA, signB;

    uA.f = a;
    uiA = uA.ui;
    uB.f = b;
    uiB = uB.ui;
    if ( isNaNF32UI( uiA ) || isNaNF32UI( uiB ) ) {
        union ui32_f32 uZ;
        uint_fast32_t uiZ;
        uiZ = softfloat_propagateF32UI( uiA, uiB );
        uZ.ui = uiZ;
        return uZ.f;
    }
    signA = signF32UI( uiA );
    signB = signF32UI( uiB );

    return ( signA != signB )
        ? ( signB ? a : b )
        : ( ((uiA != uiB) && (signB ^ (uiB < uiA))) ? a : b );
}


// -----------------------------------------------------------------------
// Public functions
// -----------------------------------------------------------------------

namespace fpu {

    void rm(uint_fast8_t mode)
    {
        softfloat_roundingMode = mode;
    }


    uint_fast8_t rm()
    {
        return softfloat_roundingMode;
    }


    void flags(uint_fast8_t flags)
    {
        softfloat_exceptionFlags = flags;
    }


    uint_fast8_t flags()
    {
        return softfloat_exceptionFlags;
    }


    float32_t f32_add(float32_t a, float32_t b)
    {
        return ftz( ::f32_add(daz(a), daz(b)) );
    }


    float32_t f32_sub(float32_t a, float32_t b)
    {
        return ftz( ::f32_sub(daz(a), daz(b)) );
    }


    float32_t f32_mul(float32_t a, float32_t b)
    {
        return ftz( ::f32_mul(daz(a), daz(b)) );
    }


    float32_t f32_div(float32_t a, float32_t b)
    {
        return ftz( ::f32_div(daz(a), daz(b)) );
    }


    float32_t f32_sqrt(float32_t a)
    {
        return ftz( ::f32_sqrt(daz(a)) );
    }


    float32_t f32_copySign(float32_t a, float32_t b)
    {
        uint32_t uA = UI32(a);
        uint32_t uB = UI32(b);
        return F32( (uA & 0x7FFFFFFF) | (uB & 0x80000000) );
    }


    float32_t f32_copySignNot(float32_t a, float32_t b)
    {
        uint32_t uA = UI32(a);
        uint32_t uB = UI32(b);
        return F32( (uA & 0x7FFFFFFF) | ((~uB) & 0x80000000) );
    }


    float32_t f32_copySignXor(float32_t a, float32_t b)
    {
        uint32_t uA = UI32(a);
        uint32_t uB = UI32(b);
        return F32( (uA & 0x7FFFFFFF) | ((uA ^ uB) & 0x80000000) );
    }


    // NB: IEEE 754-201x compatible
    float32_t f32_minNum(float32_t a, float32_t b)
    {
        return ::f32_minimumNumber( daz(a), daz(b) );
    }


    // NB: IEEE 754-201x compatible
    float32_t f32_maxNum(float32_t a, float32_t b)
    {
        return ::f32_maximumNumber(daz(a), daz(b));
    }


    bool f32_eq(float32_t a, float32_t b)
    {
        return ::f32_eq(daz(a), daz(b));
    }


    bool f32_lt(float32_t a, float32_t b)
    {
        return ::f32_lt(daz(a), daz(b));
    }


    bool f32_le(float32_t a, float32_t b)
    {
        return ::f32_le(daz(a), daz(b));
    }


    int_fast32_t f32_to_i32(float32_t a)
    {
        return ::f32_to_i32(a, softfloat_roundingMode, true);
    }


    uint_fast32_t f32_to_ui32(float32_t a)
    {
        return ::f32_to_ui32(a, softfloat_roundingMode, true);
    }


    int_fast64_t f32_to_i64(float32_t a)
    {
        return ::f32_to_i64(a, softfloat_roundingMode, true);
    }


    uint_fast64_t f32_to_ui64(float32_t a)
    {
        return ::f32_to_ui64(a, softfloat_roundingMode, true);
    }


    uint_fast16_t f32_classify(float32_t a)
    {
        return ::f32_classify(a);
    }


    float32_t i32_to_f32(int32_t a)
    {
        return ::i32_to_f32(a);
    }


    float32_t ui32_to_f32(uint32_t a)
    {
        return ::ui32_to_f32(a);
    }


    float32_t i64_to_f32(int64_t a)
    {
        return ::i64_to_f32(a);
    }


    float32_t ui64_to_f32(uint64_t a)
    {
        return ::ui64_to_f32(a);
    }


    float32_t f32_mulAdd(float32_t a, float32_t b, float32_t c)
    {
        return ftz( ::f32_mulAdd(daz(a), daz(b), daz(c)) );
    }


    float32_t f32_mulSub(float32_t a, float32_t b, float32_t c)
    {
        return ftz( ::f32_mulAdd(daz(a), daz(b), neg(daz(c))) );
    }


    float32_t f32_negMulAdd(float32_t a, float32_t b, float32_t c)
    {
        return ftz( ::f32_mulAdd(neg(daz(a)), daz(b), neg(daz(c))) );
    }

    float32_t f32_negMulSub(float32_t a, float32_t b, float32_t c)
    {
        return ftz( ::f32_mulAdd(neg(daz(a)), daz(b), daz(c)) );
    }

    // ----- Esperanto single-precision extension ----------------------------

    // Graphics downconvert

    float16_t f32_to_f16(float32_t a)
    {
        return F16( float32tofloat16(a) );
    }


    float11_t f32_to_f11(float32_t a)
    {
        return F11( float32tofloat11(a) );
    }


    float10_t f32_to_f10(float32_t a)
    {
        return F10( float32tofloat10(a) );
    }


    uint_fast32_t f32_to_un24(float32_t a)
    {
        return float32tounorm24(a);
    }


    uint_fast16_t f32_to_un16(float32_t a)
    {
        return float32tounorm16(a);
    }


    uint_fast16_t f32_to_un10(float32_t a)
    {
        return float32tounorm10(a);
    }


    uint_fast8_t f32_to_un8(float32_t a)
    {
        return float32tounorm8(a);
    }


    uint_fast8_t f32_to_un2(float32_t a)
    {
        return float32tounorm2(a);
    }


    uint_fast16_t f32_to_sn16(float32_t a)
    {
        return float32tosnorm16(a);
    }


    uint_fast8_t f32_to_sn8(float32_t a)
    {
        return float32tosnorm8(a);
    }


    // Graphics upconvert

    float32_t f16_to_f32(float16_t a)
    {
        return float16tofloat32( UI16(daz(a)) );
    }


    float32_t f11_to_f32(float11_t a)
    {
        return float11tofloat32( UI16(daz(a)) );
    }


    float32_t f10_to_f32(float10_t a)
    {
        return float10tofloat32( UI16(daz(a)) );
    }


    float32_t un24_to_f32(uint32_t a)
    {
        return unorm24tofloat32(a);
    }


    float32_t un16_to_f32(uint16_t a)
    {
        return unorm16tofloat32(a);
    }


    float32_t un10_to_f32(uint16_t a)
    {
        return unorm10tofloat32(a);
    }


    float32_t un8_to_f32(uint8_t a)
    {
        return unorm8tofloat32(a);
    }


    float32_t un2_to_f32(uint8_t a)
    {
        return unorm2tofloat32(a);
    }


    float32_t sn16_to_f32(uint16_t a)
    {
        return snorm16tofloat32(a);
    }


    float32_t sn8_to_f32(uint8_t a)
    {
        return snorm8tofloat32(a);
    }

    // Graphics additional

    float32_t f32_sin2pi(float32_t a)
    {
        return ftz( F32(ttrans_fsin(UI32(daz(a)))) );
    }


    float32_t f32_exp2(float32_t a)
    {
        return ftz( F32(ttrans_fexp2(UI32(daz(a)))) );
    }


    float32_t f32_log2(float32_t a)
    {
        return ftz( F32(ttrans_flog2(UI32(daz(a)))) );
    }


    float32_t f32_frac(float32_t a)
    {
        uint32_t uiA = UI32(daz(a));
        if (isNaNF32UI(uiA)) {
            if (softfloat_isSigNaNF32UI(uiA))
                softfloat_raiseFlags(softfloat_flag_invalid);
            return F32(defaultNaNF32UI);
        }
        float32_t z;
        {

            // FIXME: We should not use floating-point here, we should
            // implement a softfloat equivalent
            //
            // According to Khronos f32_frac() behaves like:
            //   fint = trunc(value);
            //   return copysign(isinf(value) ? 0.0 : value - fint, value);
            union { uint32_t ui; float f; } uA, uZ;
            double intpart;
            uA.ui = uiA;
            uZ.f = float(modf(double(uA.f), &intpart));
            z = F32(uZ.ui);
        }
        return ftz(z);
    }


    float32_t f32_rcp(float32_t a)
    {
        return ftz( F32(ttrans_frcp(UI32(daz(a)))) );
    }


    float32_t f32_rsqrt(float32_t a)
    {
        return ftz( F32(ttrans_frsq(UI32(daz(a)))) );
    }


    float32_t f32_roundToInt(float32_t a)
    {
        // NB: Don't convert denormals; input denormals will become 0 anyway
        return ::f32_roundToInt(a, softfloat_roundingMode, true);
    }


    float32_t f32_cubeFaceIdx(uint8_t a, float32_t b)
    {
        a &= 0x3;
        if (a == 0x3) {
            softfloat_raiseFlags(softfloat_flag_invalid);
            return F32(defaultNaNF32UI);
        }
        uint_fast32_t signB = signF32UI(UI32(b)) ? 1 : 0;
        return ui32_to_f32( (a << 1) | signB );
    }


    float32_t f32_cubeFaceSignS(uint8_t a, float32_t b)
    {
        a &= 0x7;
        return F32( ((a == 0) || (a == 5)) ? (0x80000000 | UI32(b))
                                           : (0x7FFFFFFF & UI32(b)) );
    }


    float32_t f32_cubeFaceSignT(uint8_t a, float32_t b)
    {
        a &= 0x7;
        return F32( (a == 2) ? (0x7FFFFFFF & UI32(b))
                             : (0x80000000 | UI32(b)) );
    }


    float32_t fxp1516_to_f32(int32_t a)
    {
        // convert int32 to float32 and then scale by 2^-16
        float32_t z = ::i32_to_f32(a);
        uint32_t t = UI32(z);
        return (t)
            ? F32( packToF32UI(signF32UI(t), expF32UI(t) - 16, fracF32UI(t)) )
            : z;
    }


    int32_t f32_to_fxp1714(float32_t a)
    {
        uint32_t v = UI32(a);

        // NaN converts to 0
        if (isNaNF32UI(v))
        {
            if (softfloat_isSigNaNF32UI(v))
                softfloat_raiseFlags(softfloat_flag_invalid);
            return 0;
        }
        // denormals and +/-0.0 convert to 0
        if ((v & 0x7f800000) == 0)
        {
            return 0;
        }
        // abs(val) >= 2.0769187e+34, including +/-infinity, converts to 0
        if ((v & 0x7fffffff) >= 0x78800000)
        {
            softfloat_raiseFlags(softfloat_flag_inexact);
            return 0;
        }
        // all others are converted as int(val*16384.0+0.5)
        v = v ? packToF32UI(signF32UI(v), expF32UI(v) + 14, fracF32UI(v)) : 0;
        float32_t z = ::f32_add(F32(v), { 0x3f000000 /*0.5*/ });
        return ::f32_to_i32(z, softfloat_roundingMode, true);
    }


    int32_t fxp1714_rcpStep(int32_t a, int32_t b)
    {
        // FIXME: We should not use floating-point here, we should implement a
        // softfloat equivalent

        // Input value is 2xtriArea with 15.16 precision
        double tmp = double(a) / double(1 << 16);

        double yn = double(b) / double(1 << 14);
        double fa = yn * tmp;
        uint32_t partial = uint32_t(fa * double(uint64_t(1) << 31));
        double unpartial = double(partial) / double(uint64_t(1) << 31);
        double result = yn * (2.0 - unpartial);
        return int32_t(result * double(1 << 14));
    }

    // ----- Esperanto tensor extension --------------------------------------

    float32_t f32_tensorMulAddF16(float32_t acc,
                                  float16_t a1, float16_t b1,
                                  float16_t a2, float16_t b2)
    {
        // Calculate acc += f32(a1)*f32(b1) + f32(a2)*f32(b2)
        //
        // The real accumulation is performed with "infinte" accuracy as a
        // 3-way addition
        float32_t fma1 = ::f32_mulAdd(::f16_to_f32(daz(a1)),
                                      ::f16_to_f32(daz(b1)),
                                      daz(acc));
        float32_t fma2 = ::f32_mulAdd(::f16_to_f32(daz(a2)),
                                      ::f16_to_f32(daz(b2)),
                                      fma1);
        return ftz(fma2);
    }

    // the original code for f32_tensorMulAddF16() from bemu.cpp
#if 0
#if defined(USE_REAL_TXFMA)
                    iufval mul1_a, mul1_b, fma1, mul2_a, mul2_b, fma2;

                    // 1st FMA
                    bool      mul1_a_den = ((mul1_a_hex & 0x7C00) == 0);        // detect input denormal or zero
                    int32_t   mul1_a_exp = ((mul1_a_hex >> 10) & 0x1F) - 15;    // get exponent

                    bool      mul1_b_den = ((mul1_b_hex & 0x7C00) == 0);        // detect input denormal or zero
                    int32_t   mul1_b_exp = ((mul1_b_hex >> 10) & 0x1F) - 15;    // get exponent

                    // flush denormals to zero, preserving sign
                    if (mul1_a_den) mul1_a_hex &= 0x8000;
                    if (mul1_b_den) mul1_b_hex &= 0x8000;

                    mul1_a.flt = _cvtsh_ss(mul1_a_hex);                         // convert to fp32
                    mul1_b.flt = _cvtsh_ss(mul1_b_hex);                         // convert to fp32
                    fma1.flt   = mul1_a.flt * mul1_b.flt;                       // perform first mul

                    // 2nd FMA
                    bool      mul2_a_den = ((mul2_a_hex & 0x7C00) == 0);        // detect input denormal or zero
                    int32_t   mul2_a_exp = ((mul2_a_hex >> 10) & 0x1F) - 15;    // get exponent

                    bool      mul2_b_den = ((mul2_b_hex & 0x7C00) == 0);        // detect input denormal or zero
                    int32_t   mul2_b_exp = ((mul2_b_hex >> 10) & 0x1F) - 15;    // get exponent

                    // flush denormals to zero, preserving sign
                    if (mul2_a_den) mul2_a_hex &= 0x8000;
                    if (mul2_b_den) mul2_b_hex &= 0x8000;

                    mul2_a.flt = _cvtsh_ss(mul2_a_hex);                         // convert to fp32
                    mul2_b.flt = _cvtsh_ss(mul2_b_hex);                         // convert to fp32
                    fma2.flt   = mul2_a.flt * mul2_b.flt;                       // perform second mul

                    // Get hex value and exponents of three operands of final addition
                    uint32_t hex_accum  = accum.u;
                    int32_t  accum_exp  = ((hex_accum >> 23) & 0xFF) - 127;
                    uint32_t hex_fma1   = fma1.u;
                    int32_t  fma1_exp   = ((hex_fma1 >> 23) & 0xFF) - 127;
                    int32_t  fma1_exp_r = (mul1_a_den || mul1_b_den) ? -127 : mul1_a_exp + mul1_b_exp; // use exponent without shifting to match rtl
                    uint32_t hex_fma2   = fma2.u;
                    int32_t  fma2_exp   = ((hex_fma2 >> 23) & 0xFF) - 127;
                    int32_t  fma2_exp_r = (mul2_a_den || mul2_b_den) ? -127 : mul2_a_exp + mul2_b_exp; // use exponent without shifting to match rtl

                    // Get max exponent that determines where we truncate other values
                    int32_t exp_max = ((accum_exp >= fma1_exp_r) && (accum_exp  >= fma2_exp_r)) ? accum_exp  :
                                      ((fma1_exp_r >= accum_exp) && (fma1_exp_r >= fma2_exp_r)) ? fma1_exp_r :
                                                                                                  fma2_exp_r;

                    // Truncate all values to (set truncate accordingly):
                    //    - 0: b23 (no rouding)
                    //    - 1: round bit
                    //    - 2: guard bit
                    int32_t  truncate = 1;
                    int32_t  accum_erase = exp_max - accum_exp - truncate;
                    uint32_t accum_trunc = (accum_erase > 23) ? (hex_accum &  0x80000000) :
                                           (accum_erase < 1 ) ? (hex_accum              ) :
                                                                (hex_accum & ((0xFFFFFFFF >> accum_erase) << accum_erase));
                    int32_t   fma1_erase = exp_max -  fma1_exp - truncate;
                    uint32_t  fma1_trunc = ( fma1_erase > 23) ? (hex_fma1  &  0x80000000) :
                                           ( fma1_erase < 1 ) ? (hex_fma1               ) :
                                                                (hex_fma1  & ((0xFFFFFFFF >>  fma1_erase) <<  fma1_erase));
                    int32_t   fma2_erase = exp_max -  fma2_exp - truncate;
                    uint32_t  fma2_trunc = ( fma2_erase > 23) ? (hex_fma2  &  0x80000000) :
                                           ( fma2_erase < 1 ) ? (hex_fma2               ) :
                                                                (hex_fma2  & ((0xFFFFFFFF >>  fma2_erase) <<  fma2_erase));

                    // Convert back to fp32 after truncation
                    float accum_fp32 = cast_uint32_to_float(accum_trunc);
                    float  fma1_fp32 = cast_uint32_to_float(fma1_trunc);
                    float  fma2_fp32 = cast_uint32_to_float(fma2_trunc);

                    // Perform accumulation (first in fp64 to avoid uncontrolled rounding => then clip to fp32 with appropiate rounding)
                    double    res64     = double(accum_fp32) + double(fma1_fp32) + double(fma2_fp32);
                    uint64_t  hex_res64 = cast_double_to_uint64(res64);
                    hex_res64           = hex_res64 & 0xFFFFFFFFE0000000; // Cut mantissa down to 23 bits from original 52 bits of FP64
                    res64               = cast_uint64_to_double(hex_res64);
                    iufval res;
                    res.flt             = float(res64);

                    // Finally, clear output denormals and NaNs
                    handle_denormal(res);
                    handle_nan_default(res);
                    FREGS[TFMA_MAX_BCOLS/VL*ar+bf].u[bm] = res.u;

                    DEBUG_EMU(gprintf("\tTensor FMA f%d[%d]: %g = %g + %g * %g\n",TFMA_MAX_BCOLS/VL*ar+bf,bm,res.flt,accum.flt,mul_a.flt,mul_b.flt););
                    DEBUG_EMU(gprintf("\t           f%d[%d]: 0x%08x = 0x%08x + 0x%08x * 0x%08x\n",TFMA_MAX_BCOLS/VL*ar+bf,bm,res.u,accum.u,mul_a.u,mul_b.u);)
                    // Uncomment to help debugging
                    //DEBUG_EMU(gprintf("\tBefore truncating accum 0x%08x, fma1 0x%08x, fma2 0x%08x\n", hex_accum, hex_fma1, hex_fma2);)
                    //DEBUG_EMU(gprintf("\tAfter truncating  accum 0x%08x, fma1 0x%08x, fma2 0x%08x\n", accum_trunc, fma1_trunc, fma2_trunc);)
                    //DEBUG_EMU(gprintf("\tExponents accum exp %d, fma1 exp %d, fma2 exp %d\n", accum_exp, fma1_exp, fma2_exp);)
                    //DEBUG_EMU(gprintf("\tRemove bits according to max exp %d, accum %d, fma1 %d, fma2 %d\n", exp_max, accum_erase, fma1_erase, fma2_erase);)
#else
                    iufval32 mul_a, mul_b, res;

                    // NB: we do not treat fp16 input denormals as zero here
                    // if ((mul1_a_hex & 0x7c00) == 0) mul1_a_hex &= 0x8000;
                    // if ((mul1_b_hex & 0x7c00) == 0) mul1_b_hex &= 0x8000;
                    // if ((mul2_a_hex & 0x7c00) == 0) mul2_a_hex &= 0x8000;
                    // if ((mul2_b_hex & 0x7c00) == 0) mul2_b_hex &= 0x8000;

                    // 1st FMA
                    mul_a.flt = _cvtsh_ss(mul1_a_hex);
                    mul_b.flt = _cvtsh_ss(mul1_b_hex);
                    res.flt   = fmaf(mul_a.flt, mul_b.flt, accum.flt);
                    handle_denormal(res);
                    handle_nan_default(res);
                    //FREGS[TFMA_MAX_BCOLS/VL*ar+bf].u[bm] = res.u;

                    DEBUG_EMU(gprintf("\tTensor FMA f%d[%d]: %g = %g + %g * %g\n",TFMA_MAX_BCOLS/VL*ar+bf,bm,res.flt,accum.flt,mul_a.flt,mul_b.flt););
                    DEBUG_EMU(gprintf("\t           f%d[%d]: 0x%08x = 0x%08x + 0x%04x * 0x%04x\n",TFMA_MAX_BCOLS/VL*ar+bf,bm,res.u,accum.u,mul1_a_hex,mul1_b_hex););

                    // 2nd FMA
                    accum     = res;
                    mul_a.flt = _cvtsh_ss(mul2_a_hex);
                    mul_b.flt = _cvtsh_ss(mul2_b_hex);
                    //accum.f = FREGS[FMA_MAX_BCOLS/VL*ar+bf].f[bm];
                    //handle_denormal(accum);
                    res.flt   = fmaf(mul_a.flt, mul_b.flt, accum.flt);
                    handle_denormal(res);
                    handle_nan_default(res);
                    FREGS[TFMA_MAX_BCOLS/VL*ar+bf].u[bm] = res.u;

                    DEBUG_EMU(gprintf("\tTensor FMA f%d[%d]: %g = %g + %g * %g\n",TFMA_MAX_BCOLS/VL*ar+bf,bm,res.flt,accum.flt,mul_a.flt,mul_b.flt););
                    DEBUG_EMU(gprintf("\t           f%d[%d]: 0x%08x = 0x%08x + 0x%04x * 0x%04x\n",TFMA_MAX_BCOLS/VL*ar+bf,bm,res.u,accum.u,mul2_a_hex,mul2_b_hex););
#endif
#endif

} // namespace fpu
