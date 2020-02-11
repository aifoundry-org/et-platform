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


// safe right shifts ( shifting by more than 32 bits returns 0)
template<typename T> T rshift(T v, unsigned int s) {
  if (s >= sizeof(T)*8)  return 0;
  else return v >> s;
}


float10_t f32_to_f10(float32_t a)
{
    ui32_f32 uA;
    uint_fast32_t uiA;
    uint8_t  sign;
    uint32_t mantissa32;
    uint32_t mantissa10;
    int32_t  exponent;
    bool denorm;
    bool nan;
    bool zero;
    bool infinity;
    ui16_f10 uZ;

    uA.f = a;
    uiA = uA.ui;
#ifdef SOFTFLOAT_DENORMALS_TO_ZERO
    if (isSubnormalF32UI(uiA))
        uiA = softfloat_zeroExpSigF32UI(uiA);
#endif

    //  Disassemble float32_t value into sign, exponent and mantissa.

    //  Extract sign.
    sign = ((uiA & 0x80000000) == 0) ? 0 : 1;

    //  Extract exponent.
    exponent = (uiA >> 23) & 0xff;

    //  Extract mantissa.
    mantissa32 = uiA & 0x007fffff;

    //  Compute flags.
    denorm = (exponent == 0) && (mantissa32 != 0);
    zero = (exponent == 0) && (mantissa32 == 0);
    nan = (exponent == 0xff) && (mantissa32 != 0);
    infinity = (exponent == 0xff) && (mantissa32 == 0);

    //  Flush float32_t denorms to 0
    if (denorm || zero || (sign != 0)) {
        uZ.ui = 0;
        return uZ.f;
    }
    if (nan) {
        uZ.ui = 0x03f0;
        return uZ.f;
    }
    //  Flush numbers with an exponent not representable in float10 to infinite.
    if (infinity) {
        uZ.ui = 0x03e0;
        return uZ.f;
    }
    if (exponent > (127 + 15)) {
        uZ.ui = 0x03df;
        return uZ.f;
    }

    //  Convert exponent to float10.  Excess 127 to excess 15.
    exponent = exponent - 127 + 15;

    if (exponent > 0)
    {
        //  Convert mantissa from float32_t to float10.  Mantissa 23+1 to mantissa 5+1.
        mantissa10 = (mantissa32 >> 18) & 0x001f;

        // No rounding for F10 datatypes
        // apply rounding
        //mantissa10 += (mantissa32 >> 17) & 0x1; // Round to nearest, ties to Max Magnitude
        //mantissa10 += ((mantissa32>>17) & 0x1) &
        //              (((mantissa32 & 0x5FFFF) != 0) ? 1 : 0); // Round to nearest, ties to even

        // increment exponent if there was overflow
        if ( (mantissa10 >> 5) != 0 ) {
            exponent ++;
            mantissa10 = 0;
        }
    }
    //  Check for denormalized float10 number.
    else
    {
        //  Add implicit float32_t bit to the mantissa.
        mantissa32 = mantissa32 + 0x800000;

        //  Convert mantissa from float32_t to float10.  Mantissa 23+1 to mantissa 5+1.
        mantissa10 = (mantissa32 >> 18) & 0x003f;

        //  Denormalize mantissa.
        mantissa10 = rshift(mantissa10, (-exponent + 1));

        // no rounding for F10 Datatype
        // apply rounding
        // mantissa10 += rshift(mantissa32 , (18-exponent)) & 0x1; // Round to nearest, ties to Max Magnitude
        //mantissa10 += (rshift(mantissa32, (18-exponent)) & 0x1) &
        //              ((((rshift(mantissa32, (19-exponent)) & 0x1) != 0) && ((mantissa32 & (1<<(18-exponent)-1)) != 0)) ? 1 : 0); // Round to nearest, ties to even

        // increment exponent if there was overflow
        if ( (mantissa10 >> 6) != 0 ) {
            exponent ++;
            mantissa10 = mantissa10 >> 1;
        }

        if (exponent < -18)
            mantissa10  = rshift( mantissa10, -( 18 + exponent));
        //  Set denormalized exponent.
        exponent = 0;
    }

    //  Assemble the float10 value.
    uZ.ui = (uint16_t(exponent & 0x1f) << 5) | mantissa10;
    return uZ.f;
}
