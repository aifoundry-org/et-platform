/* vim: set ts=8 sw=4 et sta cin cino=\:0s,l1,g0,N-s,E-s,i0,+2s,(0,W2s : */

#include "softfloat/platform.h"
#include "softfloat/internals.h"
#include "softfloat/specialize.h"


namespace fpu {


float32_t fxp1516_to_f32(int32_t a)
{
    ui32_f32 uZ;
    uint_fast32_t uiZ;

    // convert int32 to float32 and then scale by 2^-16
    uZ.f = ::i32_to_f32(a);
    uiZ = uZ.ui;
    uZ.ui = uiZ ? packToF32UI(signF32UI(uiZ), expF32UI(uiZ) - 16, fracF32UI(uiZ)) : 0;
    return uZ.f;
}


} // namespace fpu
