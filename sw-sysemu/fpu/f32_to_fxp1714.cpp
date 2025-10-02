/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#include "softfloat/platform.h"
#include "softfloat/internals.h"
#include "softfloat/specialize.h"


namespace fpu {


int32_t f32_to_fxp1714(float32_t a)
{
    ui32_f32 uA;
    ui32_f32 uB;
    uint_fast32_t uiA;

    uA.f = a;
    uiA = uA.ui;

    // NaN converts to 0
    if (isNaNF32UI(uiA)) {
        return 0;
    }
    // denormals and +/-0.0 convert to 0
    if ((uiA & 0x7f800000) == 0) {
        return 0;
    }
    // abs(val) >= 2.0769187e+34, including +/-infinity, converts to 0
    if ((uiA & 0x7fffffff) >= 0x78800000) {
        return 0;
    }
    // all others are converted as int(val*16384.0+0.5)
    uA.ui = uiA ? packToF32UI(signF32UI(uiA), expF32UI(uiA) + 14, fracF32UI(uiA)) : 0;
    uB.ui = 0x3f000000 /*0.5*/;
    float32_t z = ::f32_add(uA.f, uB.f);
    int32_t answer = ::f32_to_i32(z, softfloat_roundingMode, true);

    // this instruction does not generate exceptions, but f32_add() and
    // f32_to_i32() may generate exceptions
    softfloat_exceptionFlags = 0;
    return answer;
}


} // namespace fpu
