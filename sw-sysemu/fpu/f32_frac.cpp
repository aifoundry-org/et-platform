/* vim: set ts=8 sw=4 et sta cin cino=\:0s,l1,g0,N-s,E-s,i0,+2s,(0,W2s : */

#include <cmath>
#include "fpu_types.h"
#include "fpu_casts.h"
#include "softfloat/internals.h"
#include "softfloat/specialize.h"


float32_t f32_frac(float32_t a)
{
    ui32_f32 uZ;
    uint32_t uiA = fpu::UI32(a);
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
