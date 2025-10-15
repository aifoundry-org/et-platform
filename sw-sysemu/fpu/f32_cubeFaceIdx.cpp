/*-------------------------------------------------------------------------
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------*/

#include "softfloat/platform.h"
#include "softfloat/internals.h"
#include "softfloat/specialize.h"


namespace fpu {


float32_t f32_cubeFaceIdx(uint8_t a, float32_t b)
{
    ui32_f32 uB;
    uint_fast32_t uiB;
    ui32_f32 uZ;
    uint_fast32_t uiZ;

    uB.f = b;
    uiB = uB.ui;

    a &= 0x3;
    if (a == 0x3) {
        uiZ = defaultNaNF32UI;
        uZ.ui = uiZ;
        return uZ.f;
    }
    uiZ = (a << 1) | (signF32UI(uiB) ? 1 : 0);
    float32_t z = ::ui32_to_f32( uiZ );

    // this instruction does not generate exceptions, but ui32_to_f32() may
    // generate exceptions
    softfloat_exceptionFlags = 0;
    return z;
}


} // namespace fpu
