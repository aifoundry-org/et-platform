/* -*- Mode:C++; c-basic-offset: 4; -*- */

#include "fpu.h"
#include "fpu_casts.h"
#include "cvt.h"
#include "ttrans.h"
#include "softfloat/softfloat.h"
#include "softfloat/internals.h"
#include "softfloat/specialize.h"
#include "debug.h"



#ifdef FPU_DEBUG
#undef FPU_DEBUG
#endif


#include <cmath> // FIXME: remove this when we fix f32_frac()

// Extend softfloat arithmetic flags
enum {
    softfloat_flag_denormal = 128
};


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
        uint_fast32_t src = UI32(x);
        uint_fast32_t dst = subnormalToZeroF32UI(src);
        if (src != dst)
            softfloat_raiseFlags(softfloat_flag_denormal);
        return F32(dst);
#else
        return x;
#endif
    }


    static inline float16_t daz(float16_t x)
    {
#ifdef FPU_DAZ
        uint_fast16_t src = UI16(x);
        uint_fast16_t dst = subnormalToZeroF16UI(src);
        if (src != dst)
            softfloat_raiseFlags(softfloat_flag_denormal);
        return F16(dst);
#else
        return x;
#endif
    }


    static inline float11_t daz(float11_t x)
    {
#ifdef FPU_DAZ
        uint_fast16_t src = UI16(x);
        uint_fast16_t dst = subnormalToZeroF11UI(src);
        if (src != dst)
            softfloat_raiseFlags(softfloat_flag_denormal);
        return F11(dst);
#else
        return x;
#endif
    }


    static inline float10_t daz(float10_t x)
    {
#ifdef FPU_DAZ
        uint_fast16_t src = UI16(x);
        uint_fast16_t dst = subnormalToZeroF10UI(src);
        if (src != dst)
            softfloat_raiseFlags(softfloat_flag_denormal);
        return F10(dst);
#else
        return x;
#endif
    }


    static inline float32_t ftz(float32_t x)
    {
#ifdef FPU_FTZ
        uint_fast32_t src = UI32(x);
        uint_fast32_t dst = subnormalToZeroF32UI(src);
        if (src != dst)
            softfloat_raiseFlags(softfloat_flag_underflow);
        return F32(dst);
#else
        return x;
#endif
    }


    static inline float16_t ftz(float16_t x)
    {
#ifdef FPU_FTZ
        uint_fast16_t src = UI16(x);
        uint_fast16_t dst = subnormalToZeroF16UI(src);
        if (src != dst)
            softfloat_raiseFlags(softfloat_flag_underflow);
        return F16(dst);
#else
        return x;
#endif
    }

} // namespace fpu


// -----------------------------------------------------------------------
// Extensions to softfloat
// -----------------------------------------------------------------------

static uint_fast32_t
softfloat_ignoreNaNF32UI( uint_fast32_t uiA, uint_fast32_t uiB )
{
    if ( softfloat_isSigNaNF32UI( uiA ) || softfloat_isSigNaNF32UI( uiB ) ) {
        softfloat_raiseFlags( softfloat_flag_invalid );
    }
    return isNaNF32UI( uiA )
        ? ( isNaNF32UI( uiB ) ? defaultNaNF32UI : uiB )
        : uiA;
}


static uint_fast32_t
softfloat_ignoreQNaNF32UI( uint_fast32_t uiA, uint_fast32_t uiB )
{
    if ( softfloat_isSigNaNF32UI( uiA ) || softfloat_isSigNaNF32UI( uiB ) ) {
        softfloat_raiseFlags( softfloat_flag_invalid );
        return defaultNaNF32UI;
    }
    return isNaNF32UI( uiA )
        ? ( isNaNF32UI( uiB ) ? defaultNaNF32UI : uiB )
        : uiA;
}


static uint_fast32_t softfloat_propagateNaNF32UI(
    uint_fast32_t uiA, uint_fast32_t uiB, uint_fast32_t uiC )
{

    if ( softfloat_isSigNaNF32UI( uiA ) || softfloat_isSigNaNF32UI( uiB ) ||
         softfloat_isSigNaNF32UI( uiC ) ) {
        softfloat_raiseFlags( softfloat_flag_invalid );
    }
    return defaultNaNF32UI;
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
        uiZ = softfloat_ignoreNaNF32UI( uiA, uiB );
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
        uiZ = softfloat_ignoreNaNF32UI( uiA, uiB );
        uZ.ui = uiZ;
        return uZ.f;
    }
    signA = signF32UI( uiA );
    signB = signF32UI( uiB );

    return ( signA != signB )
        ? ( signB ? a : b )
        : ( ((uiA != uiB) && (signB ^ (uiB < uiA))) ? a : b );
}


static float32_t f32_minNum( float32_t a, float32_t b )
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
        uiZ = softfloat_ignoreQNaNF32UI( uiA, uiB );
        uZ.ui = uiZ;
        return uZ.f;
    }
    signA = signF32UI( uiA );
    signB = signF32UI( uiB );

    return ( signA != signB )
        ? ( signA ? a : b )
        : ( ((uiA != uiB) && (signA ^ (uiA < uiB))) ? a : b );
}


static float32_t f32_maxNum( float32_t a, float32_t b )
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
        uiZ = softfloat_ignoreQNaNF32UI( uiA, uiB );
        uZ.ui = uiZ;
        return uZ.f;
    }
    signA = signF32UI( uiA );
    signB = signF32UI( uiB );

    return ( signA != signB )
        ? ( signB ? a : b )
        : ( ((uiA != uiB) && (signB ^ (uiB < uiA))) ? a : b );
}


static uint_fast32_t f16_mulExt( float16_t a, float16_t b )
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
    uint_fast32_t sigZ;
    uint_fast32_t uiZ;
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
    sigA = sigA | 0x0400;
    sigB = sigB | 0x0400;
    sigZ = (uint_fast32_t) sigA * sigB;
#ifdef FPU_DEBUG
    std::cout << "a: " << Float16(signA,expA,sigA) << " [flags: " << SOFTFLOAT_FLAGS << "]\n";
    std::cout << "b: " << Float16(signB,expB,sigB) << " [flags: " << SOFTFLOAT_FLAGS << "]\n";
#endif
    uiZ = packToF32UI( signZ, expZ, sigZ );
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
#ifdef FPU_DEBUG
    {
        union ui32_f32 uZ;
        uZ.ui = uiZ;
        std::cout << "p: " << uZ.f << " [flags: " << SOFTFLOAT_FLAGS << "]\n";
    }
#endif
    return uiZ;
}


static float32_t f32_add3( uint_fast32_t uiA, uint_fast32_t uiB, float32_t c )
{
    bool signA;
    int_fast16_t expA;
    uint_fast32_t sigA;
    bool signB;
    int_fast16_t expB;
    uint_fast32_t sigB;
    union ui32_f32 uC;
    uint_fast32_t uiC;
    bool signC;
    int_fast16_t expC;
    uint_fast32_t sigC;
    int_fast8_t subB;
    int_fast8_t subC;
    int_fast16_t expDiffB;
    int_fast16_t expDiffC;
    union ui32_f32 uZ;
    uint_fast32_t uiZ;
    bool signZ;
    int_fast16_t expZ;
    uint_fast32_t sigZ;
    int_fast8_t shiftDist;

    /*------------------------------------------------------------------------
    *------------------------------------------------------------------------*/
    uC.f = c;
    uiC = uC.ui;
    signA = signF32UI( uiA );
    expA  = expF32UI( uiA );
    sigA  = fracF32UI( uiA );
    signB = signF32UI( uiB );
    expB  = expF32UI( uiB );
    sigB  = fracF32UI( uiB );
    signC = signF32UI( uiC );
    expC  = expF32UI( uiC );
    sigC  = fracF32UI( uiC );
    /*------------------------------------------------------------------------
    *------------------------------------------------------------------------*/
    sigA <<= 3;
    sigB <<= 3;
    if ( expC ) {
        if ( expC != 0xFF) sigC |= 0x00800000;
    } else {
        expC = ( sigC != 0 );
    }
#ifdef FPU_DEBUG
    std::cout << "a_orig: " << Float32<0>(signA,expA,sigA) << '\n';
    std::cout << "b_orig: " << Float32<0>(signB,expB,sigB) << '\n';
    std::cout << "c_orig: " << Float32<0>(signC,expC,sigC) << "\n\n";
#endif
    if ( ( expA < expB ) || ( expA == expB && sigA < sigB ) ) {
        std::swap(signA, signB);
        std::swap(expA, expB);
        std::swap(sigA, sigB);
    }
    if ( ( expA < expC ) || ( expA == expC && sigA < sigC ) ) {
        std::swap(signA, signC);
        std::swap(expA, expC);
        std::swap(sigA, sigC);
    }
    if ( ( expB < expC ) || ( expB == expC && sigB < sigC ) ) {
        std::swap(signB, signC);
        std::swap(expB, expC);
        std::swap(sigB, sigC);
    }
    /*------------------------------------------------------------------------
    *------------------------------------------------------------------------*/
    if ( expA == 0xFF ) {
        if ( sigA ) goto propagateNaN;
        if ( expB == 0xFF ) {
            if ( sigB ) goto propagateNaN;
            if ( signA ^ signB ) goto generateNaN;
            if ( expC == 0xFF ) {
                if ( sigC ) goto propagateNaN;
                if ( signA ^ signC ) goto generateNaN;
            }
        }
        uiZ = packToF32UI( signA, 0xFF, 0 );
        goto uiZ;
    }
    /*------------------------------------------------------------------------
    *------------------------------------------------------------------------*/
    sigA <<= 4;
    sigB <<= 4;
    sigC <<= 4;
#ifdef FPU_DEBUG
    std::cout << "H_norm: " << Float32<4>(signA,expA,sigA) << '\n';
    std::cout << "M_norm: " << Float32<4>(signB,expB,sigB) << '\n';
    std::cout << "L_norm: " << Float32<4>(signC,expC,sigC) << "\n\n";
#endif
    /*------------------------------------------------------------------------
    *------------------------------------------------------------------------*/
    expDiffB = expA - expB;
    expDiffC = expA - expC;
#ifdef FPU_DEBUG
    std::cout << "H_shft: " << Float32<4>(signA,expA,sigA) << " (shft: 0)\n";
#endif
    if ( expDiffB ) {
        sigB = softfloat_shiftRightJam32( sigB, expDiffB );
        sigB = (sigB & ~2) | ((sigB & 2) >> 1);
    }
    if ( expDiffC ) {
        sigC = softfloat_shiftRightJam32( sigC, expDiffC );
        sigC = (sigC & ~2) | ((sigC & 2) >> 1);
    }
    /*------------------------------------------------------------------------
    *------------------------------------------------------------------------*/
    subB = signA ^ signB;
    subC = signA ^ signC;
#ifdef FPU_DEBUG
    std::cout << "M_shft: " << Float32<4>(signB,expA,sigB) << " (shft: " << expDiffB << ")\n";
    std::cout << "L_shft: " << Float32<4>(signC,expA,sigC) << " (shft: " << expDiffC << ")\n\n";
#endif
    if ( subB && !subC && (sigB & sigC & 1) ) {
        --sigC;
    } else if ( !subB && (sigB & sigC & 1) ) {
        --sigB;
    }
    signZ = signA;
    expZ = expA;
    sigZ = sigA + (subB ? -sigB : sigB) + (subC ? -sigC : sigC);
#ifdef FPU_DEBUG
    std::cout << "H_add : " << Float32<4>(signA,expZ,sigA) << '\n';
    std::cout << "M_add : " << Float32<4>(signB,expZ,(subB ? -sigB : sigB)) << " (inv: " << (!!subB) << ")\n";
    std::cout << "L_add : " << Float32<4>(signC,expZ,(subC ? -sigC : sigC)) << " (inv: " << (!!subC) << ")\n\n";
    std::cout << "z_sum : " << Float32<4>(signZ,expZ,sigZ) << "\n\n";
#endif
    if ( sigZ >= 0x80000000 ) {
        signZ = !signZ;
        sigZ = -sigZ;
#ifdef FPU_DEBUG
        std::cout << "z_neg : " << Float32<4>(signZ,expZ,sigZ) << '\n';
#endif
    }
    if ( sigZ >= 0x10000000 ) {
        expZ += 1;
        sigZ >>= 1;
#ifdef FPU_DEBUG
        std::cout << "z_adj1: " << Float32<4>(signZ,expZ,sigZ) << '\n';
#endif
    }
    if ( sigZ >= 0x10000000 ) {
        expZ += 1;
        sigZ >>= 1;
#ifdef FPU_DEBUG
        std::cout << "z_adj2: " << Float32<4>(signZ,expZ,sigZ) << '\n';
#endif
    }
    if ( (subB || subC) && !sigZ ) {
        uiZ =
            packToF32UI(
                (softfloat_roundingMode == softfloat_round_min), 0, 0 );
#ifdef FPU_DEBUG
        std::cout << "z_cncl: " << Float32<4>(signF32UI(uiZ),expF32UI(uiZ),fracF32UI(uiZ)) << '\n';
#endif
        goto uiZ;
    }
    sigZ = ((sigZ & ~1) << 2) | (sigZ & 1);
    if ( sigZ < 0x40000000 ) {
        --expZ;
    	sigZ = ((sigZ & ~1) << 1) | (sigZ & 1);
#ifdef FPU_DEBUG
        std::cout << "z_adj3: " << Float32<7>(signZ,expZ,sigZ) << '\n';
#endif
    }
    shiftDist = softfloat_countLeadingZeros32( sigZ ) - 1;
    expZ -= shiftDist;
    sigZ = ((sigZ & ~1) << shiftDist) | (sigZ & 1);
#ifdef FPU_DEBUG
    std::cout << "z_adj4: " << Float32<7>(signZ,expZ,sigZ) << '\n';
#endif
#ifdef FPU_DEBUG
    {
        float32_t fZ;
        int_fast8_t shiftDist;
        shiftDist = softfloat_countLeadingZeros32(sigZ) - 1;
        std::cout << "z_prnd: " << Float32<>(signZ,expZ,sigZ) << " (" << int(shiftDist) << ")\n";
        if ( (7 <= shiftDist) && ((unsigned int) (expZ-shiftDist) < 0xFD) ) {
            std::cout << "z_pack: " << Float32<>(signZ,sigZ ? (expZ-shiftDist) : 0,sigZ<<(shiftDist-7)) << '\n';
        } else {
            std::cout << "z_rnd : " << Float32<>(signZ,expZ-shiftDist,sigZ<<shiftDist) << " [" << SOFTFLOAT_FLAGS << "]\n";
        }
        fZ = softfloat_normRoundPackToF32( signZ, expZ, sigZ );
        std::cout << "z_rslt: " << fZ << " [flags: " << SOFTFLOAT_FLAGS << "]\n";
    }
#endif
    return softfloat_roundPackToF32( signZ, expZ, sigZ );
    /*------------------------------------------------------------------------
    *------------------------------------------------------------------------*/
 generateNaN:
    softfloat_raiseFlags( softfloat_flag_invalid );
    uiZ = defaultNaNF32UI;
    goto uiZ;
    /*------------------------------------------------------------------------
    *------------------------------------------------------------------------*/
 propagateNaN:
    uiZ = softfloat_propagateNaNF32UI( uiA, uiB, uiC );
 uiZ:
    uZ.ui = uiZ;
#ifdef FPU_DEBUG
    std::cout << "z_rslt: " << uZ.f << " [flags: " << SOFTFLOAT_FLAGS << "]\n";
#endif
    return uZ.f;
}


static float32_t f16_mulMulAdd3(
    float32_t c, float16_t a1, float16_t b1, float16_t a2, float16_t b2 )
{
    uint_fast32_t p1;
    uint_fast32_t p2;

#ifdef FPU_DEBUG
    std::cout << "\n----- p1 = a1 * b1 --------------------------------------------------------\n";
#endif
    p1 = f16_mulExt( a1, b1 );
#ifdef FPU_DEBUG
    std::cout << "\n----- p2 = a2 * b2 --------------------------------------------------------\n";
#endif
    p2 = f16_mulExt( a2, b2 );
#ifdef FPU_DEBUG
    std::cout << "\n----- z = p1 + p2 + c -----------------------------------------------------\n";
#endif
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
    float32_t f32_minimumNumber(float32_t a, float32_t b)
    {
        return ::f32_minimumNumber(daz(a), daz(b));
    }


    // NB: IEEE 754-201x compatible
    float32_t f32_maximumNumber(float32_t a, float32_t b)
    {
        return ::f32_maximumNumber(daz(a), daz(b));
    }


    // NB: IEEE 754-2008 compatible
    float32_t f32_minNum(float32_t a, float32_t b)
    {
        return ::f32_minNum(a, b);
    }


    // NB: IEEE 754-2008 compatible
    float32_t f32_maxNum(float32_t a, float32_t b)
    {
        return ::f32_maxNum(a, b);
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
        return ::f32_to_i32(daz(a), softfloat_roundingMode, true);
    }


    uint_fast32_t f32_to_ui32(float32_t a)
    {
        return ::f32_to_ui32(daz(a), softfloat_roundingMode, true);
    }


    int_fast64_t f32_to_i64(float32_t a)
    {
        return ::f32_to_i64(daz(a), softfloat_roundingMode, true);
    }


    uint_fast64_t f32_to_ui64(float32_t a)
    {
        return ::f32_to_ui64(daz(a), softfloat_roundingMode, true);
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
        return ftz( ::f32_to_f16(daz(a)) );
    }


    float11_t f32_to_f11(float32_t a)
    {
        return F11( float32tofloat11(daz(a)) );
    }


    float10_t f32_to_f10(float32_t a)
    {
        return F10( float32tofloat10(daz(a)) );
    }


    uint_fast32_t f32_to_un24(float32_t a)
    {
        return float32tounorm24(daz(a));
    }


    uint_fast16_t f32_to_un16(float32_t a)
    {
        return float32tounorm16(daz(a));
    }


    uint_fast16_t f32_to_un10(float32_t a)
    {
        return float32tounorm10(daz(a));
    }


    uint_fast8_t f32_to_un8(float32_t a)
    {
        return float32tounorm8(daz(a));
    }


    uint_fast8_t f32_to_un2(float32_t a)
    {
        return float32tounorm2(daz(a));
    }


    uint_fast16_t f32_to_sn16(float32_t a)
    {
        return float32tosnorm16(daz(a));
    }


    uint_fast8_t f32_to_sn8(float32_t a)
    {
        return float32tosnorm8(daz(a));
    }


    // Graphics upconvert

    float32_t f16_to_f32(float16_t a)
    {
        return ::f16_to_f32(daz(a));
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


    float32_t sn24_to_f32(uint16_t a)
    {
        return snorm24tofloat32(a);
    }


    float32_t sn16_to_f32(uint16_t a)
    {
        return snorm16tofloat32(a);
    }


    float32_t sn10_to_f32(uint16_t a)
    {
        return snorm10tofloat32(a);
    }


    float32_t sn8_to_f32(uint8_t a)
    {
        return snorm8tofloat32(a);
    }


    float32_t sn2_to_f32(uint8_t a)
    {
        return snorm2tofloat32(a);
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
        return ::f32_roundToInt(daz(a), softfloat_roundingMode, true);
    }


    float32_t f32_cubeFaceIdx(uint8_t a, float32_t b)
    {
        a &= 0x3;
        if (a == 0x3) {
            softfloat_raiseFlags(softfloat_flag_invalid);
            return F32(defaultNaNF32UI);
        }
        uint_fast32_t signB = signF32UI(UI32(b)) ? 1 : 0;
        return ::ui32_to_f32( (a << 1) | signB );
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
        uint32_t v = UI32(daz(a));

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
