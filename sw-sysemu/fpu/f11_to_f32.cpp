/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#include "fpu_types.h"
#include "fpu_casts.h"
#include "softfloat/platform.h"
#include "softfloat/internals.h"
#include "softfloat/specialize.h"


float32_t f11_to_f32(float11_t a)
{
    ui16_f11 uA;
    uint_fast16_t uiA;
    uint32_t mantissa;
    int32_t  exponent;
    uint32_t normbits = 0;
    bool denorm;
    bool infinite;
    bool nan;
    bool zero;
    ui32_f32 uZ;

    uA.f = a;
    uiA = uA.ui;
#ifdef SOFTFLOAT_DENORMALS_TO_ZERO
    if (isSubnormalF11UI(uiA))
        uiA = softfloat_zeroExpSigF11UI(uiA);
#endif

    //  Read the float11 exponent.
    exponent = ((uiA & 0x07c0) >> 6);

    //  Read the float11 mantissa (explicit bits).
    mantissa = (uiA & 0x003f);

    //  Compute flags.
    denorm = (exponent == 0) && (mantissa != 0);
    zero = (exponent == 0) && (mantissa == 0);
    infinite = (exponent == 0x1f) && (mantissa == 0);
    nan = (exponent == 0x1f) && (mantissa != 0);

    //  Return zeros.
    if (zero) {
        uZ.ui = 0;
        return uZ.f;
    }

    //  Convert exponent and mantissa to float32_t.  Convert from excess 15 and convert to excess 127.
    if (infinite || nan)
        exponent = 0xff;
    else
        exponent = denorm ? (-14 + 127) : (exponent - 15) + 127;

    //  Convert mantissa to float32_t.  Mantissa goes from 6+1 bits to 23+1 bits.
    if (nan) {
        uZ.ui = defaultNaNF32UI;
        return uZ.f;
    }
    if (!infinite)
        mantissa = (mantissa << 17);

    //  Check for denormalized float16 values.
    if (denorm)
    {
        //  Renormalize mantissa for denormalized float16 values.
        if      ((mantissa & 0x00400000) != 0)
            normbits = 1;
        else if ((mantissa & 0x00200000) != 0)
            normbits = 2;
        else if ((mantissa & 0x00100000) != 0)
            normbits = 3;
        else if ((mantissa & 0x00080000) != 0)
            normbits = 4;
        else if ((mantissa & 0x00040000) != 0)
            normbits = 5;
        else if ((mantissa & 0x00020000) != 0)
            normbits = 6;
        //else
        //panic("GPUMath", "convertFP16ToFP32", "Error normalizing denormalized float16 mantissa.");

        //  Decrease exponent 1 bit.
        exponent = exponent - normbits;

        //  Shift exponent 1 bit and remove implicit bit;
        mantissa = ((mantissa << normbits) & 0x007fffff);
    }

    //  Assemble the float32_t value with the obtained sign, exponent and mantissa.
    uZ.ui = (uint32_t(exponent & 0xff) << 23) | mantissa;
    return uZ.f;
}
