/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#include "fpu_types.h"
#include "softfloat/platform.h"
#include "softfloat/internals.h"
#include "softfloat/specialize.h"
#include "debug.h"

#ifdef FPU_DEBUG
#undef FPU_DEBUG
#endif

#ifdef FPU_DEBUG
#include <cmath>
#ifdef  NEUMAIER_TOWARDZERO
#include <cfenv>
#endif
#endif


static uint_fast32_t softfloat_propagateNaNF32UI(
    uint_fast32_t uiA, uint_fast32_t uiB, uint_fast32_t uiC )
{
    if ( softfloat_isSigNaNF32UI( uiA ) || softfloat_isSigNaNF32UI( uiB ) ||
         softfloat_isSigNaNF32UI( uiC ) ) {
        softfloat_raiseFlags( softfloat_flag_invalid );
    }
    return defaultNaNF32UI;
}


static uint_fast32_t f16_mulExt( uint_fast16_t uiA, uint_fast16_t uiB )
{
    bool signA;
    int_fast8_t expA;
    uint_fast16_t sigA;
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
    signA = signF16UI( uiA );
    expA  = expF16UI( uiA );
    sigA  = fracF16UI( uiA );
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


static float32_t f32_add2( uint_fast32_t uiA, uint_fast32_t uiB )
{
    bool signA;
    int_fast16_t expA;
    uint_fast32_t sigA;
    bool signB;
    int_fast16_t expB;
    uint_fast32_t sigB;
    int_fast8_t subB;
    int_fast16_t expDiff;
    union ui32_f32 uZ;
    uint_fast32_t uiZ;
    bool signZ;
    int_fast16_t expZ;
    uint_fast32_t sigZ;
    int_fast8_t shiftDist;

    /*------------------------------------------------------------------------
    *------------------------------------------------------------------------*/
    signA = signF32UI( uiA );
    expA  = expF32UI( uiA );
    sigA  = fracF32UI( uiA );
    signB = signF32UI( uiB );
    expB  = expF32UI( uiB );
    sigB  = fracF32UI( uiB );
    /*------------------------------------------------------------------------
    *------------------------------------------------------------------------*/
    sigA <<= 7;
    sigB <<= 7;
#ifdef FPU_DEBUG
    std::cout << "a_orig: " << Float32<4>(signA,expA,sigA) << '\n';
    std::cout << "b_orig: " << Float32<4>(signB,expB,sigB) << '\n';
#endif
    if ( ( expA < expB ) || ( expA == expB && sigA < sigB ) ) {
        std::swap(signA, signB);
        std::swap(expA, expB);
        std::swap(sigA, sigB);
    }
    /*------------------------------------------------------------------------
    *------------------------------------------------------------------------*/
    if ( expA == 0xFF ) {
        if ( sigA ) goto propagateNaN;
        if ( expB == 0xFF ) {
            if ( sigB ) goto propagateNaN;
            if ( signA ^ signB ) goto generateNaN;
        }
        uiZ = packToF32UI( signA, 0xFF, 0 );
        goto uiZ;
    }
#ifdef FPU_DEBUG
    std::cout << "H_norm: " << Float32<4>(signA,expA,sigA) << '\n';
    std::cout << "M_norm: " << Float32<4>(signB,expB,sigB) << '\n';
#endif
    /*------------------------------------------------------------------------
    *------------------------------------------------------------------------*/
    expDiff = expA - expB;
#ifdef FPU_DEBUG
    std::cout << "H_shft: " << Float32<4>(signA,expA,sigA) << " (shft: 0)\n";
#endif
    if ( expDiff ) {
        sigB = softfloat_shiftRightJam32( sigB, expDiff );
        sigB = (sigB & ~2) | ((sigB & 2) >> 1);
    }
    /*------------------------------------------------------------------------
    *------------------------------------------------------------------------*/
    subB = signA ^ signB;
#ifdef FPU_DEBUG
    std::cout << "M_shft: " << Float32<4>(signB,expA,sigB) << " (shft: " << expDiff << ")\n";
#endif
    signZ = signA;
    expZ = expA;
    sigZ = sigA + (subB ? -sigB : sigB);
#ifdef FPU_DEBUG
    std::cout << "H_add : " << Float32<4>(signA,expZ,sigA) << '\n';
    std::cout << "M_add : " << Float32<4>(signB,expZ,(subB ? -sigB : sigB)) << " (inv: " << (!!subB) << ")\n";
    std::cout << "z_sum : " << Float32<4>(signZ,expZ,sigZ) << "\n\n";
#endif
    if ( sigZ >= 0x80000000 ) {
        signZ = !signZ;
        sigZ = (-sigZ & ~2);
#ifdef FPU_DEBUG
        std::cout << "z_neg : " << Float32<4>(signZ,expZ,sigZ) << '\n';
#endif
    }
    if ( sigZ >= 0x10000000 ) {
        expZ += 1;
        sigZ = ((sigZ >> 1) & ~2) | ((sigZ >> 2) & 1) | (sigZ & 1);
#ifdef FPU_DEBUG
        std::cout << "z_adj1: " << Float32<4>(signZ,expZ,sigZ) << '\n';
#endif
    }
    if ( sigZ >= 0x10000000 ) {
        expZ += 1;
        sigZ = ((sigZ >> 1) & ~2) | ((sigZ >> 2) & 1) | (sigZ & 1);
#ifdef FPU_DEBUG
        std::cout << "z_adj2: " << Float32<4>(signZ,expZ,sigZ) << '\n';
#endif
    }
    if ( !sigZ ) {
        if ( subB ) {
            uiZ =
                packToF32UI(
                    (softfloat_roundingMode == softfloat_round_min), 0, 0 );
            goto uiZ;
        } else if ( !expDiff ) {
            uiZ = packToF32UI(signZ, expZ, 0);
            goto uiZ;
        }
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
    {
        float32_t fZ;
        fZ = softfloat_roundPackToF32( signZ, expZ, sigZ );
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
    uiZ = softfloat_propagateNaNF32UI( uiA, uiB );
 uiZ:
    uZ.ui = uiZ;
#ifdef FPU_DEBUG
    std::cout << "z_rslt: " << uZ.f << " [flags: " << SOFTFLOAT_FLAGS << "]\n";
#endif
    return uZ.f;
}


static float32_t f32_add3( uint_fast32_t uiA, uint_fast32_t uiB, uint_fast32_t uiC )
{
    bool signA;
    int_fast16_t expA;
    uint_fast32_t sigA;
    bool signB;
    int_fast16_t expB;
    uint_fast32_t sigB;
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
    uint_fast64_t sigZ;
    int_fast8_t shiftDist;

    /*------------------------------------------------------------------------
    *------------------------------------------------------------------------*/
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
        if ( expC != 0xFF ) sigC |= 0x00800000;
    } else {
        expC = ( sigC != 0 );
    }
#ifdef FPU_DEBUG
    std::cout << "a_orig: " << Float32<0>(signA,expA,sigA) << '\n';
    std::cout << "b_orig: " << Float32<0>(signB,expB,sigB) << '\n';
    std::cout << "c_orig: " << Float32<0>(signC,expC,sigC) << "\n\n";
#endif
    if ( (expC >= expA) && (expC >= expB) ) {
        std::swap( signA, signC );
        std::swap( expA, expC );
        std::swap( sigA, sigC );
    }
    else if ( (expA >= expB) && (expA >= expC) ) {
        /* do nothing */
    }
    else if ( (expB >= expA) && (expB >= expC) ) {
        std::swap( signA, signB );
        std::swap( expA, expB );
        std::swap( sigA, sigB );
    }
    /*------------------------------------------------------------------------
    *------------------------------------------------------------------------*/
    if ( expA == 0xFF ) {
        if ( expB == 0xFF ) {
            if ( !sigA && !sigB && (signA ^ signB) ) goto generateNaN;
            if ( expC == 0xFF ) {
                if ( !sigA && !sigC && (signA ^ signC) ) goto generateNaN;
                if ( !sigB && !sigC && (signB ^ signC) ) goto generateNaN;
                if ( sigC ) goto propagateNaN;
            }
            if ( sigB ) goto propagateNaN;
        }
        else if ( expC == 0xFF ) {
            if ( !sigA && !sigC && (signA ^ signC) ) goto generateNaN;
            if ( sigC ) goto propagateNaN;
        }
        if ( sigA ) goto propagateNaN;
        uiZ = packToF32UI( signA, 0xFF, 0 );
        goto uiZ;
    }
    /*------------------------------------------------------------------------
    *------------------------------------------------------------------------*/
    sigA <<= 5;
    sigB <<= 5;
    sigC <<= 5;
#ifdef FPU_DEBUG
    std::cout << "L_norm: " << Float32<5>(signC,expC,sigC) << '\n';
    std::cout << "M_norm: " << Float32<5>(signB,expB,sigB) << '\n';
    std::cout << "H_norm: " << Float32<5>(signA,expA,sigA) << "\n\n";
#endif
    /*------------------------------------------------------------------------
    *------------------------------------------------------------------------*/
    expDiffB = expA - expB;
    expDiffC = expA - expC;
    if ( expDiffB ) {
        sigB = softfloat_shiftRightJam32( sigB, expDiffB );
        sigB = (sigB & ~7) | ((sigB & 4) >> 2) | ((sigB & 2) >> 1) | (sigB & 1);
    }
    if ( expDiffC ) {
        sigC = softfloat_shiftRightJam32( sigC, expDiffC );
        sigC = (sigC & ~7) | ((sigC & 4) >> 2) | ((sigC & 2) >> 1) | (sigC & 1);
    }
    /*------------------------------------------------------------------------
    *------------------------------------------------------------------------*/
    subB = signA ^ signB;
    subC = signA ^ signC;
#ifdef FPU_DEBUG
    std::cout << "L_shft: " << Float32<5>(signC,expA,sigC) << " (shft: " << expDiffC << ")\n";
    std::cout << "M_shft: " << Float32<5>(signB,expA,sigB) << " (shft: " << expDiffB << ")\n";
    std::cout << "H_shft: " << Float32<5>(signA,expA,sigA) << " (shft: 0)\n\n";
#endif
    signZ = signA;
    expZ = expA;
#ifdef FPU_DEBUG
    std::cout << "L_neg : " << Float32<5>(signC,expZ,(subC ? ((~sigC & ~3) | (sigC & 3)) : sigC)) << " (inv: " << (!!subC) << ")" << ((subC && !(sigC & 3)) ? " +1\n" : "\n");
    std::cout << "M_neg : " << Float32<5>(signB,expZ,(subB ? ((~sigB & ~7) | (sigB & 3)) : sigB)) << " (inv: " << (!!subB) << ")" << ((subB && !(sigB & 3)) ? " +8\n" : "\n");
    std::cout << "H_neg : " << Float32<5>(signA,expZ,sigA) << "\n\n";
#endif
    if ( subB ) {
        sigB = (~sigB & ~7) + (sigB & 3);
        if ( !(sigB & 3) ) {
            sigB += 8;
        }
    }
    if ( subC ) {
        sigC = -sigC & ~2;
    }
    sigZ = sigA + sigB + sigC;
    sigZ = (sigZ & ~2ull) | ((sigZ & 2) >> 1);
#ifdef FPU_DEBUG
    std::cout << "L_add : " << Float32<5>(signC,expZ,sigC) << " (inv: " << (!!subC) << ")\n";
    std::cout << "M_add : " << Float32<5>(signB,expZ,sigB) << " (inv: " << (!!subB) << ")\n";
    std::cout << "H_add : " << Float32<5>(signA,expZ,sigA) << "\n\n";
    std::cout << "z_sum : " << Float32<5>(signZ,expZ,sigZ) << "\n\n";
#endif
    if ( (sigZ & ~3ull) == 0 ) {
        if ( sigZ & 3 ) {
            softfloat_raiseFlags( softfloat_flag_inexact );
        }
        if ( subB || subC ) {
            uiZ = packToF32UI( sigZ ? signZ : 0, 0, 0 );
        } else {
            uiZ = packToF32UI( signZ, 0, 0 );
        }
        goto uiZ;
    }
    if ( ~(sigZ | 3) == 0 ) {
        if ( sigZ & 3 ) {
            softfloat_raiseFlags( softfloat_flag_inexact );
        }
        uiZ = packToF32UI( !signZ, 0, 0 );
        goto uiZ;
    }
    if ( sigZ >= 0x100000000ull ) {
        signZ = !signZ;
        sigZ = (-sigZ & ~2ull);
#ifdef FPU_DEBUG
        std::cout << "z_neg : " << Float32<5>(signZ,expZ,sigZ) << '\n';
#endif
    }
    while ( sigZ >= 0x20000000 ) {
        expZ += 1;
        sigZ = ((sigZ >> 1) & ~2) | ((sigZ >> 2) & 1) | (sigZ & 1);
#ifdef FPU_DEBUG
        std::cout << "z_adj1: " << Float32<5>(signZ,expZ,sigZ) << '\n';
#endif
    }
    --expZ;
    sigZ = ((sigZ & ~1) << 2) | (sigZ & 1);
#ifdef FPU_DEBUG
    std::cout << "z_adj2: " << Float32<7>(signZ,expZ,sigZ) << '\n';
#endif
    shiftDist = softfloat_countLeadingZeros32( sigZ ) - 1;
    if ( shiftDist > 25) {
        expZ -= 25;
        sigZ = 0x40000000 | (sigZ & 1);
    } else {
        expZ -= shiftDist;
        sigZ = ((sigZ & ~0x1f) << shiftDist) | (sigZ & 0x1f);
    }
#ifdef FPU_DEBUG
    std::cout << "z_adj3: " << Float32<7>(signZ,expZ,sigZ) << " (" << int(shiftDist) << ")\n";
    {
        float32_t fZ;
        fZ = softfloat_roundPackToF32( signZ, expZ, sigZ );
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


float32_t f1632_mulAdd2(
    float16_t a1, float16_t b1, float16_t a2, float16_t b2 )
{
    union ui16_f16 uA1;
    uint_fast16_t uiA1;
    union ui16_f16 uA2;
    uint_fast16_t uiA2;
    union ui16_f16 uB1;
    uint_fast16_t uiB1;
    union ui16_f16 uB2;
    uint_fast16_t uiB2;
    uint_fast32_t uiP1;
    uint_fast32_t uiP2;

    /*------------------------------------------------------------------------
    *------------------------------------------------------------------------*/
    uA1.f = a1;
    uiA1 = uA1.ui;
    uA2.f = a2;
    uiA2 = uA2.ui;
    uB1.f = b1;
    uiB1 = uB1.ui;
    uB2.f = b2;
    uiB2 = uB2.ui;
#ifdef SOFTFLOAT_DENORMALS_TO_ZERO
    if ( isSubnormalF16UI( uiA1 ) ) {
        softfloat_raiseFlags( softfloat_flag_denormal );
        uiA1 = softfloat_zeroExpSigF16UI( uiA1 );
    }
    if ( isSubnormalF16UI( uiB1 ) ) {
        softfloat_raiseFlags( softfloat_flag_denormal );
        uiB1 = softfloat_zeroExpSigF16UI( uiB1 );
    }
    if ( isSubnormalF16UI( uiA2 ) ) {
        softfloat_raiseFlags( softfloat_flag_denormal );
        uiA2 = softfloat_zeroExpSigF16UI( uiA2 );
    }
    if ( isSubnormalF16UI( uiB2 ) ) {
        softfloat_raiseFlags( softfloat_flag_denormal );
        uiB2 = softfloat_zeroExpSigF16UI( uiB2 );
    }
#endif
    /*------------------------------------------------------------------------
    *------------------------------------------------------------------------*/
#ifdef FPU_DEBUG
    std::cout << "\n----- p1 = a1 * b1 --------------------------------------------------------\n";
#endif
    uiP1 = f16_mulExt( uiA1, uiB1 );
#ifdef FPU_DEBUG
    std::cout << "\n----- p2 = a2 * b2 --------------------------------------------------------\n";
#endif
    uiP2 = f16_mulExt( uiA2, uiB2 );
#ifdef FPU_DEBUG
    std::cout << "\n----- z = p1 + p2 ---------------------------------------------------------\n";
#endif
    return f32_add2( uiP1, uiP2 );
}


#ifdef FPU_DEBUG
static float32_t f32_add3_ieee754(float32_t a, float32_t b, float32_t c)
{
    float32_t t = f32_add(a, b);
    float32_t d = f32_add(t, c);
    return d;
}


static float32_t f32_add3_neumaier(float32_t src1, float32_t src2, float32_t src3)
{
    union {
        uint32_t ui32;
        float    fp32;
    } fs1, fs2, fs3, fd;

    fs1.ui32 = src1.v;
    fs2.ui32 = src2.v;
    fs3.ui32 = src3.v;

    float sum = 0.0;
    float c = 0.0;

    float t = sum + fs1.fp32;
    if (fabs(sum) >= fabs(fs1.fp32)) {
        c += (sum - t) + fs1.fp32;
    } else {
        c += (fs1.fp32 - t) + sum;
    }
    sum = t;

    t = sum + fs2.fp32;
    if (fabs(sum) >= fabs(fs2.fp32)) {
        c += (sum - t) + fs2.fp32;
    } else {
        c += (fs2.fp32 - t) + sum;
    }
    sum = t;

    t = sum + fs3.fp32;
    if (fabs(sum) >= fabs(fs3.fp32)) {
        c += (sum - t) + fs3.fp32;
    } else {
        c += (fs3.fp32 - t) + sum;
    }
    sum = t;

#ifdef NEUMAIER_TOWARDZERO
    fesetround(FE_TOWARDZERO);
    fd.fp32 = sum + c;
    fesetround(FE_TONEAREST);
#else
    fd.fp32 = sum + c;
#endif
    return float32_t { fd.ui32 };
}
#endif


float32_t f1632_mulAdd3(
    float16_t a1, float16_t b1, float16_t a2, float16_t b2, float32_t c )
{
    union ui16_f16 uA1;
    uint_fast16_t uiA1;
    union ui16_f16 uA2;
    uint_fast16_t uiA2;
    union ui16_f16 uB1;
    uint_fast16_t uiB1;
    union ui16_f16 uB2;
    uint_fast16_t uiB2;
    union ui32_f32 uC;
    uint_fast32_t uiC;
    uint_fast32_t uiP1;
    uint_fast32_t uiP2;

    /*------------------------------------------------------------------------
    *------------------------------------------------------------------------*/
    uA1.f = a1;
    uiA1 = uA1.ui;
    uA2.f = a2;
    uiA2 = uA2.ui;
    uB1.f = b1;
    uiB1 = uB1.ui;
    uB2.f = b2;
    uiB2 = uB2.ui;
    uC.f = c;
    uiC = uC.ui;
#ifdef SOFTFLOAT_DENORMALS_TO_ZERO
    if ( isSubnormalF16UI( uiA1 ) ) {
        softfloat_raiseFlags( softfloat_flag_denormal );
        uiA1 = softfloat_zeroExpSigF16UI( uiA1 );
    }
    if ( isSubnormalF16UI( uiB1 ) ) {
        softfloat_raiseFlags( softfloat_flag_denormal );
        uiB1 = softfloat_zeroExpSigF16UI( uiB1 );
    }
    if ( isSubnormalF16UI( uiA2 ) ) {
        softfloat_raiseFlags( softfloat_flag_denormal );
        uiA2 = softfloat_zeroExpSigF16UI( uiA2 );
    }
    if ( isSubnormalF16UI( uiB2 ) ) {
        softfloat_raiseFlags( softfloat_flag_denormal );
        uiB2 = softfloat_zeroExpSigF16UI( uiB2 );
    }
    if ( isSubnormalF32UI( uiC ) ) {
        softfloat_raiseFlags( softfloat_flag_denormal );
        uiC = softfloat_zeroExpSigF32UI( uiC );
    }
#endif
    /*------------------------------------------------------------------------
    *------------------------------------------------------------------------*/
#ifdef FPU_DEBUG
    std::cout << "\n----- p1 = a1 * b1 --------------------------------------------------------\n";
#endif
    uiP1 = f16_mulExt( uiA1, uiB1 );
#ifdef FPU_DEBUG
    std::cout << "\n----- p2 = a2 * b2 --------------------------------------------------------\n";
#endif
    uiP2 = f16_mulExt( uiA2, uiB2 );
#ifdef FPU_DEBUG
    std::cout << "\n----- z = p1 + p2 + c -----------------------------------------------------\n";
#endif
#ifdef FPU_DEBUG
    {
        uint_fast32_t round = softfloat_roundingMode;
        uint_fast32_t flags = softfloat_exceptionFlags;

        softfloat_roundingMode = softfloat_round_minMag;
        softfloat_exceptionFlags = 0;
        float32_t p1 = f32_mul(f16_to_f32(a1), f16_to_f32(b1));
        float32_t p2 = f32_mul(f16_to_f32(a2), f16_to_f32(b2));
        std::cout << "INFO testbench:\n";
        std::cout << "INFO testbench:  " << a1 << " * " << b1 << " + " << a2 << " * " << b2 << '\n';
        softfloat_exceptionFlags = 0;
        std::cout << "INFO testbench:    (p1 + p2) + c = " << f32_add3_ieee754(p1, p2, c) << '\n';
        softfloat_exceptionFlags = 0;
        std::cout << "INFO testbench:    (p1 + c) + p2 = " << f32_add3_ieee754(p1, c, p2) << '\n';
        softfloat_exceptionFlags = 0;
        std::cout << "INFO testbench:    (p2 + c) + p1 = " << f32_add3_ieee754(p2, c, p1) << '\n';
        softfloat_exceptionFlags = 0;
        std::cout << "INFO testbench:    p1 + p2 + p3  = " << f32_add3_neumaier(p1, p2, c) << '\n';

        softfloat_exceptionFlags = flags;
        softfloat_roundingMode = round;
    }
#endif
    float32_t rslt = f32_add3( uiP1, uiP2, uiC );
#ifdef FPU_DEBUG
    std::cout << "INFO testbench:    txfma16(bemu) = " << rslt << " [flags: " << SOFTFLOAT_FLAGS << "]\n";
#endif
    return rslt;
}
