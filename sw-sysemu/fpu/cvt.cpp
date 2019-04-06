/* vim: set ts=8 sw=4 et sta cin cino=\:0s,l1,g0,N-s,E-s,i0,+2s,(0,W2s : */

#include <cmath>
#include "fpu_types.h"
#include "fpu_casts.h"
#include "softfloat/platform.h"
#include "softfloat/internals.h"
#include "softfloat/specialize.h"

#if 0
#include "debug.h"

void debug_cvt(bool sign, int_fast16_t exp, uint_fast32_t sig)
{
    uint_fast8_t roundIncrement, roundBits;
    bool roundNearEven;
    roundIncrement = 0x40;
    roundNearEven = (softfloat_roundingMode == softfloat_round_near_even);
    if ( ! roundNearEven && (softfloat_roundingMode != softfloat_round_near_maxMag) ) {
        roundIncrement = (softfloat_roundingMode == (sign ? softfloat_round_min : softfloat_round_max))
                ? 0x7F
                : 0;
    }
    roundBits = sig & 0x7F;
    std::cout << "\tnumber       : " << Float32<7>(sign, exp, sig) << "\n";
    std::cout << "\trounIncrement: " << Float32<7>(sign, 0, roundIncrement) << "\n";
    std::cout << "\troundBits    : " << Float32<7>(sign, 0, roundBits) << "\n";
    sig = (sig + roundIncrement) >> 7;
    std::cout << "\tsig_shft     : " << Float32<0>(sign, exp, sig) << "\n";
    sig &= ~(uint_fast32_t) (! (roundBits ^ 0x40) & roundNearEven);
    std::cout << "\tsig_adj      : " << Float32<0>(sign, exp, sig) << "\n";
}
#endif

#if 0
    {
        bool sign = false;
        uint_fast8_t roundIncrement, roundBits;
        bool roundNearEven;
        uint_fast32_t sigZ = sig;
        roundIncrement = 0x40;
        roundNearEven = (softfloat_roundingMode == softfloat_round_near_even);
        if ( ! roundNearEven && (softfloat_roundingMode != softfloat_round_near_maxMag) ) {
            roundIncrement = (softfloat_roundingMode == (sign ? softfloat_round_min : softfloat_round_max))
                    ? 0x7F
                    : 0;
        }
        roundBits = sigZ & 0x7F;
        std::cout << "un16_to_f32: number       : " << Float32<7>(0, exp, sigZ) << "\n";
        std::cout << "un16_to_f32: rounIncrement: " << Float32<7>(0, 0, roundIncrement) << "\n";
        std::cout << "un16_to_f32: roundBits    : " << Float32<7>(0, 0, roundBits) << "\n";
        sigZ = (sigZ + roundIncrement) >> 7;
        std::cout << "un16_to_f32: sig_shft     : " << Float32<0>(0, exp, sigZ) << "\n";
        sigZ &= ~(uint_fast32_t) (! (roundBits ^ 0x40) & roundNearEven);
        std::cout << "un16_to_f32: sig_adj      : " << Float32<0>(0, exp, sigZ) << "\n";
    }
#endif

namespace fpu {


float32_t un24_to_f32(uint32_t a)
{
    ui32_f32 uZ;
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
    ui32_f32 uZ;
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
    ui32_f32 uZ;
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
    ui32_f32 uZ;
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
    ui32_f32 uZ;
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
    ui32_f32 uZ;
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
    ui32_f32 uZ;
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
    ui32_f32 uZ;
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
    ui32_f32 uZ;
    a &= 3;
    uZ.ui = packToF32UI( (bool)(a >> 1), (a ? 0x7F : 0), 0 );
    return uZ.f;
}


uint32_t f32_to_un24(float32_t val)
{
    int32_t val_i = int32_t(val.v);
    if (isNaNF32UI(val.v)) {
        return 0x00ffffff;
    }
    if (val_i >= 0x3f800000) {
        return 0x00ffffff;
    }
    if (val_i <= 0) {
        return 0x00000000;
    }
    uint32_t delta = (1 << 24) - 1;
    double ratio = double(fpu::FLT(val)) * double(delta);
    return uint32_t(ratio + 0.5f);
}


uint16_t f32_to_un16(float32_t val)
{
    int32_t val_i = int32_t(val.v);
    if (isNaNF32UI(val.v)) {
        return 0xffff;
    }
    if (val_i >= 0x3f800000) {
        return 0xffff;
    }
    if (val_i <= 0) {
        return 0x0000;
    }
    uint32_t delta = (1 << 16) - 1;
    double ratio = double(fpu::FLT(val)) * double(delta);
    return uint16_t(ratio + 0.5f);
}


uint16_t f32_to_un10(float32_t val)
{
    int32_t val_i = int32_t(val.v);
    if (isNaNF32UI(val.v)) {
        return 0x03ff;
    }
    if (val_i >= 0x3f800000) {
        return 0x03ff;
    }
    if (val_i <= 0) {
        return 0x0000;
    }
    uint32_t delta = (1 << 10) - 1;
    double ratio = double(fpu::FLT(val)) * double(delta);
    return uint16_t(ratio + 0.5f);
}


uint8_t f32_to_un8(float32_t val)
{
    int32_t val_i = int32_t(val.v);
    if (isNaNF32UI(val.v)) {
        return 0xff;
    }
    if (val_i >= 0x3f800000) {
        return 0xff;
    }
    if (val_i <= 0) {
        return 0x00;
    }
    uint32_t delta = (1 << 8) - 1;
    double ratio = double(fpu::FLT(val)) * double(delta);
    return uint8_t(ratio + 0.5f);
}


uint8_t f32_to_un2(float32_t val)
{
    int32_t val_i = int32_t(val.v);
    if (isNaNF32UI(val.v)) {
        return 0x03;
    }
    if (val_i >= 0x3f800000) {
        return 0x03;
    }
    if (val_i <= 0) {
        return 0x00;
    }
    uint32_t delta = (1 << 2) - 1;
    double ratio = double(fpu::FLT(val)) * double(delta);
    return uint8_t(ratio + 0.5f);
}


uint32_t f32_to_sn24(float32_t val)
{
    int32_t val_i = int32_t(val.v);
    if (isNaNF32UI(val.v)) {
        return 0x007fffff;
    }
    if (val_i >= 0x3f800000) {
        return 0x007fffff;
    }
    if (val.v >= 0xbf800000) {
        return 0x00800001;
    }
    float int_val = round(fpu::FLT(val) * float((1 << 23) - 1));
    int32_t res = int32_t(int_val);
    return uint32_t(res & 0x00ffffff);
}


uint16_t f32_to_sn16(float32_t val)
{
    int32_t val_i = int32_t(val.v);
    if (isNaNF32UI(val.v)) {
        return 0x7fff;
    }
    if (val_i >= 0x3f800000) {
        return 0x7fff;
    }
    if (val.v >= 0xbf800000) {
        return 0x8001;
    }
    float int_val = round(fpu::FLT(val) * float((1 << 15) - 1));
    int32_t res = int32_t(int_val);
    return uint16_t(res & 0x0000ffff);
}


uint8_t f32_to_sn8(float32_t val)
{
    int32_t val_i = int32_t(val.v);
    if (isNaNF32UI(val.v)) {
        return 0x7f;
    }
    if (val_i >= 0x3f800000) {
        return 0x7f;
    }
    if (val.v >= 0xbf800000) {
        return 0x81;
    }
    float int_val = round(fpu::FLT(val) * float((1 << 7) - 1));
    int32_t res = int32_t(int_val);
    return uint8_t(res & 0x000000ff);
}


} // namespace fpu
