/* -*- Mode:C++; c-basic-offset: 4; -*- */

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


float32_t f16_mulExt( float16_t a, float16_t b )
{
    union ui16_f16 uA;
    uint_fast16_t uiA;
    bool signA;
    int_fast8_t expA;
    uint_fast16_t sigA;
    union ui16_f16 uB;
    uint_fast16_t uiB;
    bool signB;
    int_fast8_t expB;
    uint_fast16_t sigB;
    bool signZ;
    uint_fast16_t magBits;
    struct exp8_sig16 normExpSig;
    int_fast16_t expZ;
    uint_fast32_t sigZ, uiZ;
    union ui32_f32 uZ;
    struct commonNaN commonNaN __attribute__((unused));

    /*------------------------------------------------------------------------
    *------------------------------------------------------------------------*/
    uA.f = a;
    uiA = uA.ui;
    signA = signF16UI( uiA );
    expA  = expF16UI( uiA );
    sigA  = fracF16UI( uiA );
    uB.f = b;
    uiB = uB.ui;
    signB = signF16UI( uiB );
    expB  = expF16UI( uiB );
    sigB  = fracF16UI( uiB );
    signZ = signA ^ signB;
    /*------------------------------------------------------------------------
    *------------------------------------------------------------------------*/
    if ( expA == 0x1F ) {
        if ( sigA || ((expB == 0x1F) && sigB) ) goto propagateNaN;
        magBits = expB | sigB;
        goto infArg;
    }
    if ( expB == 0x1F ) {
        if ( sigB ) goto propagateNaN;
        magBits = expA | sigA;
        goto infArg;
    }
    /*------------------------------------------------------------------------
    *------------------------------------------------------------------------*/
    if ( ! expA ) {
        if ( ! sigA ) goto zero;
        normExpSig = softfloat_normSubnormalF16Sig( sigA );
        expA = normExpSig.exp;
        sigA = normExpSig.sig;
    }
    if ( ! expB ) {
        if ( ! sigB ) goto zero;
        normExpSig = softfloat_normSubnormalF16Sig( sigB );
        expB = normExpSig.exp;
        sigB = normExpSig.sig;
    }
    /*------------------------------------------------------------------------
    *------------------------------------------------------------------------*/
    expZ = expA + expB - 0xF + 0x70;
    sigA = (sigA | 0x0400)<<4;
    sigB = (sigB | 0x0400)<<5;
    sigZ = (uint_fast32_t) sigA * sigB;
    if ( sigZ < 0x40000000 ) {
        --expZ;
        sigZ <<= 1;
    }
    uiZ = packToF32UI( signZ, expZ, sigZ>>7 );
    goto uiZ;
    /*------------------------------------------------------------------------
    *------------------------------------------------------------------------*/
 propagateNaN:
    uiZ = softfloat_propagateNaNF16UI( uiA, uiB );
    softfloat_f16UIToCommonNaN( uiZ, &commonNaN );
    uiZ = softfloat_commonNaNToF32UI( &commonNaN );
    goto uiZ;
    /*------------------------------------------------------------------------
    *------------------------------------------------------------------------*/
 infArg:
    if ( ! magBits ) {
        softfloat_raiseFlags( softfloat_flag_invalid );
        uiZ = defaultNaNF32UI;
    } else {
        uiZ = packToF32UI( signZ, 0xFF, 0 );
    }
    goto uiZ;
    /*------------------------------------------------------------------------
    *------------------------------------------------------------------------*/
 zero:
    uiZ = packToF32UI( signZ, 0, 0 );
 uiZ:
    uZ.ui = uiZ;
    return uZ.f;
}


static float32_t f32_add3(float32_t a, float32_t b, float32_t c)
{
    union ui32_f32 uA;
    uint_fast32_t uiA;
    int_fast16_t expA;
    uint_fast32_t sigA;
    union ui32_f32 uB;
    uint_fast32_t uiB;
    int_fast16_t expB;
    uint_fast32_t sigB;
    union ui32_f32 uC;
    uint_fast32_t uiC;
    int_fast16_t expC;
    uint_fast32_t sigC;
    int_fast8_t sigSubB;
    int_fast8_t sigSubC;
    int_fast16_t expDiffB;
    int_fast16_t expDiffC;
    union ui32_f32 uZ;
    uint_fast32_t uiZ;
    bool signZ;
    int_fast16_t expZ;
    uint_fast32_t sigZ;

    /*------------------------------------------------------------------------
    *------------------------------------------------------------------------*/
    uA.f = a;
    uiA = uA.ui;
    uB.f = b;
    uiB = uB.ui;
    uC.f = c;
    uiC = uC.ui;
    if ( (uiA & 0x7FFFFFFF) < (uiB & 0x7FFFFFFF) ) {
        uiA = uB.ui;
        uiB = uA.ui;
        uA.ui = uiA;
        uB.ui = uiB;
    }
    if ( (uiA & 0x7FFFFFFF) < (uiC & 0x7FFFFFFF) ) {
        uiA = uC.ui;
        uiC = uA.ui;
        uA.ui = uiA;
        uC.ui = uiC;
    }
    if ( (uiB & 0x7FFFFFFF) < (uiC & 0x7FFFFFFF) ) {
        uiB = uC.ui;
        uiC = uB.ui;
        uB.ui = uiB;
        uC.ui = uiC;
    }
    expA  = expF32UI( uiA );
    sigA  = fracF32UI( uiA );
    expB  = expF32UI( uiB );
    sigB  = fracF32UI( uiB );
    expC  = expF32UI( uiC );
    sigC  = fracF32UI( uiC );
    /*------------------------------------------------------------------------
    *------------------------------------------------------------------------*/
    if ( expA == 0xFF ) {
        if ( sigA ) goto propagateNaN;
        if ( expB == 0xFF ) {
            if ( sigB ) goto propagateNaN;
            if ( signF32UI( uiA ) ^ signF32UI( uiB ) ) goto generateNaN;
            if ( expC == 0xFF ) {
                if ( sigC ) goto propagateNaN;
                if ( signF32UI( uiA ) ^ signF32UI( uiC ) ) goto generateNaN;
            }
        }
        uiZ = packToF32UI( signF32UI( uiA ), 0xFF, 0 );
        goto uiZ;
    }
    /*------------------------------------------------------------------------
    *------------------------------------------------------------------------*/
    sigA <<= 3;
    sigB <<= 3;
    sigC <<= 3;
    if ( expA ) { sigA |= 0x04000000; } else { expA = ( sigA != 0 ); }
    if ( expB ) { sigB |= 0x04000000; } else { expB = ( sigB != 0 ); }
    if ( expC ) { sigC |= 0x04000000; } else { expC = ( sigC != 0 ); }
    /*------------------------------------------------------------------------
    *------------------------------------------------------------------------*/
    expDiffB = expA - expB;
    expDiffC = expA - expC;
    if ( expDiffB ) {
        if ( expDiffB < 31 ) {
            uint32_t stickyB = sigB << (-expDiffB & 31);
            if ( stickyB )
                softfloat_raiseFlags( softfloat_flag_inexact );
            sigB = (sigB >> expDiffB) | (stickyB != 0);
        } else {
            if ( sigB )
                softfloat_raiseFlags( softfloat_flag_inexact );
            sigB = (sigB != 0);
        }
    }
    if ( expDiffC ) {
        if ( expDiffC < 31 ) {
            uint32_t stickyC = sigC << (-expDiffC & 31);
            if ( stickyC )
                softfloat_raiseFlags( softfloat_flag_inexact );
            sigC = (sigC >> expDiffC) | (stickyC != 0);
        } else {
            if ( sigC )
                softfloat_raiseFlags( softfloat_flag_inexact );
            sigC = (sigC != 0);
        }
    }
    /*------------------------------------------------------------------------
    *------------------------------------------------------------------------*/
    sigSubB = signF32UI( uiA ) ^ signF32UI( uiB );
    sigSubC = signF32UI( uiA ) ^ signF32UI( uiC );
    signZ = signF32UI( uiA );
    expZ = expA;
    sigZ = sigA + (sigSubB ? -sigB : sigB) + (sigSubC ? -sigC : sigC);
    if ( (sigSubB || sigSubC) && sigZ >= 0x80000000 ) {
        signZ = !signZ;
        sigZ = -sigZ;
    }
    if ( !(sigSubB || sigSubC) && sigZ < 0x08000000 ) {
        --expZ;
        sigZ <<= 1;
    }
    if ( (sigSubB || sigSubC) && !sigZ ) {
        uiZ =
            packToF32UI(
                        (softfloat_roundingMode == softfloat_round_min), 0, 0 );
        goto uiZ;
    }
#ifdef F32_ADD3_HANDLE_SOME_CANCELATION
    if ( sigSubB && expDiffB == 0 && sigZ == 1 && expDiffC > 25 ) {
        uiZ =
            packToF32UI(
                        expC ? signF32UI(uiC) : (softfloat_roundingMode == softfloat_round_min),
                        expC ? expC - 1 : 0,
                        expC ? sigZ<<23 : 0 );
        goto uiZ;
    }
#endif
    sigZ <<= 3;
    if (sigZ >= 0x80000000) {
        sigZ >>= 1;
        expZ += 1;
    }
    return softfloat_normRoundPackToF32( signZ, expZ, sigZ );
    /*------------------------------------------------------------------------
    *------------------------------------------------------------------------*/
 generateNaN:
    softfloat_raiseFlags( softfloat_flag_invalid );
    uiZ = defaultNaNF32UI;
    goto uiZ;
    /*------------------------------------------------------------------------
    *------------------------------------------------------------------------*/
 propagateNaN:
    uiZ = softfloat_propagateNaNF32UI( uiA, uiB );
 uiZ:
    uZ.ui = uiZ;
    return uZ.f;
}


static float32_t f16_mulMulAdd3(
    float32_t c, float16_t a1, float16_t b1, float16_t a2, float16_t b2 )
{
    float32_t p1;
    float32_t p2;

    p1 = f16_mulExt( a1, b1 );
    p2 = f16_mulExt( a2, b2 );
    return f32_add3( p1, p2, c );
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
        return ftz(f16_mulMulAdd3(daz(acc),
                                  daz(a1), daz(b1),
                                  daz(a2), daz(b2)));
    }

} // namespace fpu
