/*-------------------------------------------------------------------------
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------*/

#include <cmath>

#include "gold.h"
#include "fpu/fpu_casts.h"

#define infinityF32UI      0x7F800000
#define minusInfinityF32UI 0xFF800000
#define defaultNaNF32UI    0x7FC00000

namespace gld {


static inline void handle_nan_default(float32_t& a)
{
    if (isNaN(a.v))
        a.v = defaultNaNF32UI;
}


static inline void handle_denormal(float32_t& a)
{
    if ((a.v & 0x7f800000) == 0)
        a.v &= 0x80000000; // preserve sign
}


float32_t f32_rsqrt(float32_t a)
{
    handle_denormal(a);
    float32_t z = fpu::F2F32(float(double(1.0) / sqrt(double(fpu::FLT(a)))));
    handle_nan_default(z);
    handle_denormal(z);
    return z;
}


float32_t f32_sin2pi(float32_t a)
{
    handle_denormal(a);

    //Take care of special cases ruined by modf
    switch (a.v) {
    case minusInfinityF32UI:
    case infinityF32UI:
        a.v = defaultNaNF32UI;
        return a;
    case 0:
    case 0x80000000:
        return a;
    }

    double dummy;
    float tmp = modf(double(fpu::FLT(a)), &dummy);
    tmp =
          tmp <= -0.75 ?  tmp + 1.0 // -IV  Quartile
        : tmp <= -0.5  ? -tmp - 0.5 // -III Quartile
        : tmp <= -0.25 ? -tmp - 0.5 // -II  Quartile
        : tmp <= -0.0  ?  tmp       // -I   Quartile
        : tmp >= 0.75  ?  tmp - 1.0 // +IV  Quartile
        : tmp >= 0.5   ? -tmp + 0.5 // +III Quartile
        : tmp >= 0.25  ? -tmp + 0.5 // +II  Quartile
        : tmp;                      // +I   Quartile

    float32_t z;
    switch (fpu::F2UI32(tmp)) {
    case 0xbe800000:
        z.v = 0xbf800000;
        break;
    case 0x00000000:
        z.v = 0x00000000;
        break;
    case 0x80000000:
        z.v = 0x80000000;
        break;
    case 0x3e800000:
        z.v = 0x3f800000;
        break;
    default:
        z = fpu::F2F32( float(sin(2.0 * M_PI * double(tmp))) );
        handle_nan_default(z);
        handle_denormal(z);
        break;
    }
    return z;
}


float32_t f32_exp2(float32_t a)
{
    handle_denormal(a);
    float32_t z = fpu::F2F32(exp2f(fpu::FLT(a)));
    handle_nan_default(z);
    handle_denormal(z);
    return z;
}


float32_t f32_log2(float32_t a)
{
    handle_denormal(a);
    float32_t z = fpu::F2F32(float(log2(double(fpu::FLT(a)))));
    handle_nan_default(z);
    handle_denormal(z);
    return z;
}


float32_t f32_rcp(float32_t a)
{
    handle_denormal(a);
    float32_t z = fpu::F2F32(float(double(1.0) / double(fpu::FLT(a))));
    handle_nan_default(z);
    handle_denormal(z);
    return z;
}


int32_t fxp1714_rcpStep(int32_t a, int32_t b)
{
    // Input value is 2xtriArea with 15.16 precision
    double fval_a = double(a) / double(1 << 16);
    double fval_b = double(b) / double(1 << 14);
    // Result value is 17.14
    double fres = (2*fval_b - fval_b*fval_b*fval_a) * double(1 << 14);
    return int32_t(fres);
}


} // namespace gld
