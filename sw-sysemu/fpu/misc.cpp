/* vim: set ts=8 sw=4 et sta cin cino=\:0s,l1,g0,N-s,E-s,i0,+2s,(0,W2s : */

#include <cmath>
#include "fpu_types.h"
#include "fpu_casts.h"
#include "softfloat/internals.h"
#include "softfloat/specialize.h"


namespace fpu {


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
    return t ? F32( packToF32UI(signF32UI(t), expF32UI(t) - 16, fracF32UI(t)) )
             : z;
}


int32_t f32_to_fxp1714(float32_t a)
{
    uint32_t v = UI32(a);

    // NaN converts to 0
    if (isNaNF32UI(v)) {
        if (softfloat_isSigNaNF32UI(v))
            softfloat_raiseFlags(softfloat_flag_invalid);
        return 0;
    }
    // denormals and +/-0.0 convert to 0
    if ((v & 0x7f800000) == 0) {
        if (v) softfloat_raiseFlags(softfloat_flag_denormal);
        return 0;
    }
    // abs(val) >= 2.0769187e+34, including +/-infinity, converts to 0
    if ((v & 0x7fffffff) >= 0x78800000) {
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


} // namespace fpu
