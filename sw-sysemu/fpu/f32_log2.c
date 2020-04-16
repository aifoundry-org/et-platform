/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#include "softfloat/platform.h"
#include "softfloat/internals.h"
#include "softfloat/specialize.h"
#include "softfloat/softfloat.h"

#include "tlog.h"

float32_t f32_log2( float32_t a )
{
    union ui32_f32 uA;
    uint_fast32_t uiA;
    bool signA;
    int_fast16_t expA;
    uint_fast32_t sigA;

    uint_fast16_t index;
    uint_fast32_t x2;
    uint_fast64_t c2, c1, c0;
    uint_fast64_t fma1, fma2, product;
    uint_fast32_t x1;
    uint_fast8_t shiftDist;
    uint_fast16_t exponent;

    bool signZ;
    int_fast16_t expZ;
    uint_fast32_t sigZ;
    uint_fast32_t uiZ;
    union ui32_f32 uZ;

    /*------------------------------------------------------------------------
    *------------------------------------------------------------------------*/
    uA.f = a;
    uiA = uA.ui;
#ifdef SOFTFLOAT_DENORMALS_TO_ZERO
    if ( isSubnormalF32UI( uiA ) ) {
        softfloat_raiseFlags( softfloat_flag_denormal );
        uiA = softfloat_zeroExpSigF32UI( uiA );
    }
#endif
    signA = signF32UI( uiA );
    expA  = expF32UI( uiA );
    sigA  = fracF32UI( uiA );
    /*------------------------------------------------------------------------
    *------------------------------------------------------------------------*/
    if ( !expA && !sigA ) {
        uiZ = packToF32UI(1, 0xFF, 0);
        goto uiZ;
    }
    if ( (expA == 0xFF) && sigA ) {
        if ( softfloat_isSigNaNF32UI( uiA ) )
            softfloat_raiseFlags( softfloat_flag_invalid );
        uiZ = defaultNaNF32UI;
        goto uiZ;
    }
    if ( signA ) {
        softfloat_raiseFlags( softfloat_flag_invalid );
        uiZ = defaultNaNF32UI;
        goto uiZ;
    }
    if ( expA == 0xFF ) {
        uiZ = uiA;
        goto uiZ;
    }
    /*------------------------------------------------------------------------
    *------------------------------------------------------------------------*/
    if ( sigA ) {
        index = (sigA >> 17) & 0x3F;
        c2 = tlog[index].c2;
        c1 = tlog[index].c1;
        c0 = tlog[index].c0;
        x2 = sigA & 0x1FFFF;
        fma1 = (c2*x2 - c1 + 0x10) & 0xfffffffffffffff0;
        fma2 = fma1*x2 + c0 + 0x80000000;
    } else {
        if ( expA == 0x7F ) {
            uiZ = 0;
            goto uiZ;
        }
        fma2 = 0;
    }
    /*------------------------------------------------------------------------
    *------------------------------------------------------------------------*/
    x1 = ( sigA >> 22 ) ? ( -sigA & 0x7FFFFF ) : ( sigA << 1 );
    product = ( 0x2000000 | (0x1FFFFFF & (fma2 >> 32)) ) * x1;

    exponent = ( (expA >= 0x7F) ? (expA + 1) : (~expA - !!sigA) ) & 0x7F;

      //cancel bits to match rtl datapath    
    if (((exponent&0x7F)!=0x0) || (((exponent&0x7F)==0x0) && (expA+( sigA >> 22 )) != 0x7f))
      product &= 0xffffffffffffff80;

    if ( expA >= 0x7F ) {
        product = ( sigA >> 22 ) ? -product : product;
    } else {
        product = ( sigA >> 22 ) ? product : -(product & 0xffffffffffffff80);
    }
    product = ( ((uint_fast64_t)exponent) << 49 ) | ( 0x1FFFFFFFFFFFFULL & product );
    /*------------------------------------------------------------------------
    *------------------------------------------------------------------------*/
    shiftDist = softfloat_countLeadingZeros32( product >> 25 );
    signZ = ( expA < 0x7F );
    expZ = 0x86 - shiftDist;
    sigZ = 0x7FFFFF & ( product >> (33 - shiftDist) );
    uiZ = packToF32UI( signZ, expZ, sigZ );
    /*------------------------------------------------------------------------
    *------------------------------------------------------------------------*/
 uiZ:
    uZ.ui = uiZ;
    return uZ.f;
}
