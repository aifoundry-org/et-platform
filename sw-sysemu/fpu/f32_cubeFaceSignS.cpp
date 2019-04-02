/* vim: set ts=8 sw=4 et sta cin cino=\:0s,l1,g0,N-s,E-s,i0,+2s,(0,W2s : */

#include "softfloat/platform.h"
#include "softfloat/internals.h"
#include "softfloat/specialize.h"


namespace fpu {


float32_t f32_cubeFaceSignS(uint8_t a, float32_t b)
{
    ui32_f32 uB;
    uint_fast32_t uiB;
    ui32_f32 uZ;

    uB.f = b;
    uiB = uB.ui;

    a &= 0x7;
    uZ.ui = ((a == 0) || (a == 5))
        ? (0x80000000 | uiB) : (0x7FFFFFFF & uiB);
    return uZ.f;
}


} // namespace fpu
