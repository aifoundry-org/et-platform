/*-------------------------------------------------------------------------
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------*/

#include <cmath>
#include "fpu_types.h"
#include "fpu_casts.h"
#include "softfloat/platform.h"
#include "softfloat/internals.h"
#include "softfloat/specialize.h"

namespace fpu {


float32_t un24_to_f32(uint32_t a)
{
    union ui32_f32 uZ;
    int_fast8_t shiftDist;
    int_fast16_t exp;
    uint_fast64_t sig;

    a &= 0xFFFFFF;
    if ( !a ) {
        uZ.ui = 0;
        return uZ.f;
    }
    if ( a == 0xFFFFFF ) {
        uZ.ui = packToF32UI(0, 0x7F, 0);
        return uZ.f;
    }
    sig = ( (uint_fast64_t)a << 40 )
        | ( (uint_fast64_t)a << 16 )
        | softfloat_shiftRightJam32(a, 8);
    if ( a & 0x800000 ) {
        sig = softfloat_shiftRightJam64( sig, 33 );
        exp = 0x7D;
    } else {
        shiftDist = softfloat_countLeadingZeros32( a ) - 9;
        exp = 0x7C - shiftDist;
        sig = softfloat_shiftRightJam64( sig, 32 - shiftDist );
    }
    uZ.f = softfloat_roundPackToF32( 0, exp, (uint_fast32_t)sig );
    softfloat_exceptionFlags = 0;
    return uZ.f;
}


float32_t un16_to_f32(uint16_t a)
{
    union ui32_f32 uZ;
    int_fast8_t shiftDist;
    int_fast16_t exp;
    uint_fast64_t sig;

    if ( !a ) {
        uZ.ui = 0;
        return uZ.f;
    }
    if ( a == 0xFFFF ) {
        uZ.ui = packToF32UI(0, 0x7F, 0);
        return uZ.f;
    }
    sig = ( (uint_fast64_t)a << 32 ) | ( (uint_fast64_t)a << 16 ) | a;
    if ( a & 0x8000 ) {
        sig = softfloat_shiftRightJam64( sig, 17 );
        exp = 0x7D;
    } else {
        shiftDist = softfloat_countLeadingZeros16( a ) - 1;
        exp = 0x7C - shiftDist;
        sig = softfloat_shiftRightJam64( sig, 16 - shiftDist );
    }
    uZ.f = softfloat_roundPackToF32( 0, exp, (uint_fast32_t)sig );
    softfloat_exceptionFlags = 0;
    return uZ.f;
}


float32_t un10_to_f32(uint16_t a)
{
    union ui32_f32 uZ;
    int_fast8_t shiftDist;
    int_fast16_t exp;
    uint_fast64_t sig;

    a &= 0x3FF;
    if ( !a ) {
        uZ.ui = 0;
        return uZ.f;
    }
    if ( a == 0x3FF ) {
        uZ.ui = packToF32UI(0, 0x7F, 0);
        return uZ.f;
    }
    sig = ( (uint_fast64_t)a << 20 ) | ( (uint_fast64_t)a << 10 ) | a;
    sig = ( sig << 30 ) | sig;
    if ( a & 0x200 ) {
        sig = softfloat_shiftRightJam64( sig, 29 );
        exp = 0x7D;
    } else {
        shiftDist = softfloat_countLeadingZeros16( a ) - 7;
        exp = 0x7C - shiftDist;
        sig = softfloat_shiftRightJam64( sig, 28 - shiftDist );
    }
    uZ.f = softfloat_roundPackToF32( 0, exp, (uint_fast32_t)sig );
    softfloat_exceptionFlags = 0;
    return uZ.f;
}


float32_t un8_to_f32(uint8_t a)
{
    union ui32_f32 uZ;
    int_fast8_t shiftDist;
    int_fast16_t exp;
    uint_fast64_t sig;

    if ( !a ) {
        uZ.ui = 0;
        return uZ.f;
    }
    if ( a == 0xFF ) {
        uZ.ui = packToF32UI(0, 0x7F, 0);
        return uZ.f;
    }
    sig = ( (uint_fast64_t)a << 8 ) | a;
    sig = ( sig << 32 ) | ( sig << 16 ) | sig;
    if ( a & 0x80 ) {
        sig = softfloat_shiftRightJam64( sig, 17 );
        exp = 0x7D;
    } else {
        shiftDist = softfloat_countLeadingZeros16( a ) - 9;
        exp = 0x7C - shiftDist;
        sig = softfloat_shiftRightJam64( sig, 16 - shiftDist );
    }
    uZ.f = softfloat_roundPackToF32( 0, exp, (uint_fast32_t)sig );
    softfloat_exceptionFlags = 0;
    return uZ.f;
}


float32_t un2_to_f32(uint8_t a)
{
    union ui32_f32 uZ;
    int_fast16_t exp;

    a &= 0x3;
    if ( !a ) {
        uZ.ui = 0;
        return uZ.f;
    }
    if ( a == 3 ) {
        uZ.ui = packToF32UI(0, 0x7F, 0);
        return uZ.f;
    }
    exp = 0x7C + ( a >> 1 );
    uZ.f = softfloat_roundPackToF32( 0, exp, 0x55555555 );
    softfloat_exceptionFlags = 0;
    return uZ.f;
}


float32_t sn16_to_f32(uint16_t a)
{
    union ui32_f32 uZ;
    int_fast8_t shiftDist;
    bool sign;
    int_fast16_t exp;
    uint_fast64_t sig;

    if ( !a ) {
        uZ.ui = 0;
        return uZ.f;
    }
    sign = (bool)( a >> 15 );
    if ( sign ) {
        a = ( a == 0x8000 ) ? 0x7FFF : ( -a & 0x7FFF );
    }
    if ( a == 0x7FFF ) {
        uZ.ui = packToF32UI(sign, 0x7F, 0);
        return uZ.f;
    }
    sig = ( (uint_fast64_t)a << 30 ) | ( (uint_fast64_t)a << 15 ) | a;
    shiftDist = softfloat_countLeadingZeros16( a ) - 1;
    exp = 0x7D - shiftDist;
    sig = softfloat_shiftRightJam64( sig, 14 - shiftDist );
    uZ.f = softfloat_roundPackToF32( sign, exp, (uint_fast32_t)sig );
    softfloat_exceptionFlags = 0;
    return uZ.f;
}


// Used by the TBOX
float32_t sn10_to_f32(uint16_t a)
{
    union ui32_f32 uZ;
    int_fast8_t shiftDist;
    bool sign;
    int_fast16_t exp;
    uint_fast64_t sig;

    if ( !a ) {
        uZ.ui = 0;
        return uZ.f;
    }
    sign = (bool)( a >> 9 );
    if ( sign ) {
        a = ( a == 0x200 ) ? 0x1FF : ( -a & 0x1FF );
    }
    if ( a == 0x1FF ) {
        uZ.ui = packToF32UI(sign, 0x7F, 0);
        return uZ.f;
    }
    sig = ( (uint_fast64_t)a << 36 ) | ( (uint_fast64_t)a << 27 )
        | ( (uint_fast64_t)a << 18 ) | ( (uint_fast64_t)a << 9 ) | a;
    shiftDist = softfloat_countLeadingZeros16( a ) - 7;
    exp = 0x7D - shiftDist;
    sig = softfloat_shiftRightJam64( sig, 14 - shiftDist );
    uZ.f = softfloat_roundPackToF32( sign, exp, (uint_fast32_t)sig );
    softfloat_exceptionFlags = 0;
    return uZ.f;
}


float32_t sn8_to_f32(uint8_t a)
{
    union ui32_f32 uZ;
    int_fast8_t shiftDist;
    bool sign;
    int_fast16_t exp;
    uint_fast64_t sig;

    if ( !a ) {
        uZ.ui = 0;
        return uZ.f;
    }
    sign = (bool)( a >> 7 );
    if ( sign ) {
        a = ( a == 0x80 ) ? 0x7F : ( -a & 0x7F );
    }
    if ( a == 0x7F ) {
        uZ.ui = packToF32UI(sign, 0x7F, 0);
        return uZ.f;
    }
    sig = ( (uint_fast64_t)a << 14 ) | ( (uint_fast64_t)a << 7 ) | a;
    sig = ( sig << 21 ) | sig;
    shiftDist = softfloat_countLeadingZeros16( a ) - 9;
    exp = 0x7D - shiftDist;
    sig = softfloat_shiftRightJam64( sig, 11 - shiftDist );
    uZ.f = softfloat_roundPackToF32( sign, exp, (uint_fast32_t)sig );
    softfloat_exceptionFlags = 0;
    return uZ.f;
}


// Used by the TBOX
float32_t sn2_to_f32(uint8_t a)
{
    union ui32_f32 uZ;
    a &= 3;
    uZ.ui = packToF32UI( (bool)(a >> 1), (a ? 0x7F : 0), 0 );
    return uZ.f;
}


uint_fast32_t f32_to_un24(float32_t a)
{
    union ui32_f32 uA;
    uint_fast32_t uiA;
    bool sign;
    int_fast16_t exp;
    uint_fast32_t sig;
    uint_fast64_t sig64;

    uA.f = a;
    uiA = uA.ui;
    sign = signF32UI( uiA );
    exp = expF32UI( uiA );
    sig = fracF32UI( uiA );

    if ( exp >= 0x7F ) {
        return ( sign && ((exp != 0xFF) || !sig) ) ? 0 : 0xFFFFFF;
    }
    if ( sign || !exp ) return 0;

    sig64 = (uint_fast64_t) (sig | 0x00800000) * 0xFFFFFF00;
    sig64 = softfloat_shiftRightJam64( sig64, 0x92 - exp );
    return softfloat_roundToUI32(
            sign, sig64, softfloat_round_near_maxMag, false );
}


uint_fast16_t f32_to_un16(float32_t a)
{
    union ui32_f32 uA;
    uint_fast32_t uiA;
    bool sign;
    int_fast16_t exp;
    uint_fast32_t sig;
    uint_fast64_t sig64;

    uA.f = a;
    uiA = uA.ui;
    sign = signF32UI( uiA );
    exp = expF32UI( uiA );
    sig = fracF32UI( uiA );

    if ( exp >= 0x7F ) {
        return ( sign && ((exp != 0xFF) || !sig) ) ? 0 : 0xFFFF;
    }
    if ( sign || !exp ) return 0;

    sig64 = (uint_fast64_t) (sig | 0x00800000) * 0xFFFF0000;
    sig64 = softfloat_shiftRightJam64( sig64, 0x9A - exp );
    return softfloat_roundToUI32(
            sign, sig64, softfloat_round_near_maxMag, false );
}


uint_fast16_t f32_to_un10(float32_t a)
{
    union ui32_f32 uA;
    uint_fast32_t uiA;
    bool sign;
    int_fast16_t exp;
    uint_fast32_t sig;
    uint_fast64_t sig64;

    uA.f = a;
    uiA = uA.ui;
    sign = signF32UI( uiA );
    exp = expF32UI( uiA );
    sig = fracF32UI( uiA );

    if ( exp >= 0x7F ) {
        return ( sign && ((exp != 0xFF) || !sig) ) ? 0 : 0x3FF;
    }
    if ( sign || !exp ) return 0;

    sig64 = (uint_fast64_t) (sig | 0x00800000) * 0xFFC00000;
    sig64 = softfloat_shiftRightJam64( sig64, 0xA0 - exp );
    return softfloat_roundToUI32(
            sign, sig64, softfloat_round_near_maxMag, false );
}


uint_fast8_t f32_to_un8(float32_t a)
{
    union ui32_f32 uA;
    uint_fast32_t uiA;
    bool sign;
    int_fast16_t exp;
    uint_fast32_t sig;
    uint_fast64_t sig64;

    uA.f = a;
    uiA = uA.ui;
    sign = signF32UI( uiA );
    exp = expF32UI( uiA );
    sig = fracF32UI( uiA );

    if ( exp >= 0x7F ) {
        return ( sign && ((exp != 0xFF) || !sig) ) ? 0 : 0xFF;
    }
    if ( sign || !exp ) return 0;

    sig64 = (uint_fast64_t) (sig | 0x00800000) * 0xFF000000;
    sig64 = softfloat_shiftRightJam64( sig64, 0xA2 - exp );
    return softfloat_roundToUI32(
            sign, sig64, softfloat_round_near_maxMag, false );
}


uint_fast8_t f32_to_un2(float32_t a)
{
    union ui32_f32 uA;
    uint_fast32_t uiA;
    bool sign;
    int_fast16_t exp;
    uint_fast32_t sig;
    uint_fast64_t sig64;

    uA.f = a;
    uiA = uA.ui;
    sign = signF32UI( uiA );
    exp = expF32UI( uiA );
    sig = fracF32UI( uiA );

    if ( exp >= 0x7F ) {
        return ( sign && ((exp != 0xFF) || !sig) ) ? 0 : 0x3;
    }
    if ( sign || !exp ) return 0;

    sig64 = (uint_fast64_t) (sig | 0x00800000) * 0xC0000000;
    sig64 = softfloat_shiftRightJam64( sig64, 0xA8 - exp );
    return softfloat_roundToUI32(
            sign, sig64, softfloat_round_near_maxMag, false );
}


uint_fast32_t f32_to_sn24(float32_t a)
{
    union ui32_f32 uA;
    uint_fast32_t uiA;
    bool sign;
    int_fast16_t exp;
    uint_fast32_t sig;
    uint_fast64_t sig64;

    uA.f = a;
    uiA = uA.ui;
    sign = signF32UI( uiA );
    exp = expF32UI( uiA );
    sig = fracF32UI( uiA );

    if ( !exp ) return 0;
    if ( exp >= 0x7F ) {
        if ( exp == 0xFF ) return ( !sig && sign ) ? 0x800001 : 0x7FFFFF;
        return sign ? 0x800001 : 0x7FFFFF;
    }

    sig64 = (uint_fast64_t) (sig | 0x00800000) * 0xFFFFFE00;
    sig64 = softfloat_shiftRightJam64( sig64, 0x93 - exp );
    return softfloat_roundToI32(
            sign, sig64, softfloat_round_near_maxMag, false ) & 0xFFFFFF;
}


uint_fast16_t f32_to_sn16(float32_t a)
{
    union ui32_f32 uA;
    uint_fast32_t uiA;
    bool sign;
    int_fast16_t exp;
    uint_fast32_t sig;
    uint_fast64_t sig64;

    uA.f = a;
    uiA = uA.ui;
    sign = signF32UI( uiA );
    exp = expF32UI( uiA );
    sig = fracF32UI( uiA );

    if ( !exp ) return 0;
    if ( exp >= 0x7F ) {
        if ( exp == 0xFF ) return ( !sig && sign ) ? 0x8001 : 0x7FFF;
        return sign ? 0x8001 : 0x7FFF;
    }

    sig64 = (uint_fast64_t) (sig | 0x00800000) * 0xFFFE0000;
    sig64 = softfloat_shiftRightJam64( sig64, 0x9B - exp );
    return softfloat_roundToI32(
            sign, sig64, softfloat_round_near_maxMag, false ) & 0xFFFF;
}


uint_fast8_t f32_to_sn8(float32_t a)
{
    union ui32_f32 uA;
    uint_fast32_t uiA;
    bool sign;
    int_fast16_t exp;
    uint_fast32_t sig;
    uint_fast64_t sig64;

    uA.f = a;
    uiA = uA.ui;
    sign = signF32UI( uiA );
    exp = expF32UI( uiA );
    sig = fracF32UI( uiA );

    if ( !exp ) return 0;
    if ( exp >= 0x7F ) {
        if ( exp == 0xFF ) return ( !sig && sign ) ? 0x81 : 0x7F;
        return sign ? 0x81 : 0x7F;
    }

    sig64 = (uint_fast64_t) (sig | 0x00800000) * 0xFE000000;
    sig64 = softfloat_shiftRightJam64( sig64, 0xA3 - exp );
    return softfloat_roundToI32(
            sign, sig64, softfloat_round_near_maxMag, false ) & 0xFF;
}


} // namespace fpu
