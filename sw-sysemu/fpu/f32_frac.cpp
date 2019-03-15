/* vim: set ts=8 sw=4 et sta cin cino=\:0s,l1,g0,N-s,E-s,i0,+2s,(0,W2s : */

#include <cmath>
#include "fpu_types.h"
#include "fpu_casts.h"
#include "softfloat/internals.h"
#include "softfloat/specialize.h"


float32_t f32_frac(float32_t a)
{
    uint32_t uiA = fpu::UI32(a);
    if (isNaNF32UI(uiA)) {
        if (softfloat_isSigNaNF32UI(uiA))
            softfloat_raiseFlags(softfloat_flag_invalid);
        return fpu::F32(defaultNaNF32UI);
    }

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
    return fpu::F32(uZ.ui);
}
