/*-------------------------------------------------------------------------
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------*/

#include <cmath>
#include "fpu_casts.h"
#include "softfloat/platform.h"
#include "softfloat/internals.h"
#include "softfloat/specialize.h"


float32_t f32_frac(float32_t a)
{
    ui32_f32 uZ;
    uint32_t uiA = fpu::UI32(a);
#ifdef SOFTFLOAT_DENORMALS_TO_ZERO
    if ( isSubnormalF32UI( uiA ) ) {
        softfloat_raiseFlags( softfloat_flag_denormal );
        uiA = softfloat_zeroExpSigF32UI( uiA );
    }
#endif
    if (isNaNF32UI(uiA)) {
        if (softfloat_isSigNaNF32UI(uiA))
            softfloat_raiseFlags(softfloat_flag_invalid);
        uZ.ui = defaultNaNF32UI;
        return uZ.f;
    }

    // FIXME: We should not use floating-point here, we should
    // implement a softfloat equivalent
    //
    // According to Khronos f32_frac() behaves like:
    //   fint = trunc(value);
    //   return copysign(isinf(value) ? 0.0 : value - fint, value);
    double intpart;
    float res = float(modf(double(fpu::FLT(uiA)), &intpart));
    return fpu::F2F32(res);
}
