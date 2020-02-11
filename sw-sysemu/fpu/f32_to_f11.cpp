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


float11_t f32_to_f11(float32_t a)
{
    ui32_f32 uA;
    uint_fast32_t uiA;
    uint8_t  sign;
    uint32_t mantissa32;
    uint32_t mantissa11;
    int32_t  exponent;
    bool denorm;
    bool nan;
    bool zero;
    bool infinity;
    ui16_f11 uZ;

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
        uZ.ui = 0x07e0;
        return uZ.f;
    }
    if (infinity) {
        uZ.ui = 0x07c0;
        return uZ.f;
    }
    //  Flush numbers with an exponent not representable in float11 to infinite.
    if (exponent > (127 + 15)) {
        uZ.ui = 0x07bf;
        return uZ.f;
    }

    //  Convert exponent to float11.  Excess 127 to excess 15.
    exponent = exponent - 127 + 15;

    if (exponent > 0)
    {
        //  Convert mantissa from float32_t to float11.  Mantissa 23+1 to mantissa 6+1.
        mantissa11 = (mantissa32 >> 17) & 0x003f;

        // no rounding for F11 Datatype
        // apply rounding
        // mantissa11 += (mantissa32 >> 16) & 0x1; // Round to nearest, ties to Max Magnitude
        //mantissa11 += ((mantissa32>>16) & 0x1) &
        //              (((mantissa32 & 0x2FFFF) != 0) ? 1 : 0); // Round to nearest, ties to even

        // increment exponent if there was overflow
        if ( (mantissa11 >> 6) != 0 ) {
            exponent ++;
            mantissa11 = 0;
        }
    }
    //  Check for denormalized float11 number.
    else
    {
        //  Add implicit float32_t bit to the mantissa.
        mantissa32 = mantissa32 + 0x800000;

        //  Convert mantissa from float32_t to float16.  Mantissa 23+1 to mantissa 10+1.
        mantissa11 = (mantissa32 >> 17) & 0x07f;

        //  Denormalize mantissa.
        mantissa11 = rshift(mantissa11, -exponent + 1);

        // No rounding for F11 datatypes
        // apply rounding
        //mantissa11 += rshift (mantissa32, (17-exponent)) & 0x1; // Round to nearest, ties to Max Magnitude
        //mantissa11 += (rshift(mantissa32,(17-exponent)) & 0x1) &
        //              ((((rshift(mantissa32,(18-exponent)) & 0x1) != 0) && ((mantissa32 & (1<<(17-exponent)-1)) != 0)) ? 1 : 0); // Round to nearest, ties to even

        // increment exponent if there was overflow
        if ( (mantissa11 >> 7) != 0 ) {
            exponent ++;
            mantissa11 = mantissa11 >> 1;
        }

        if (exponent < -19)
            mantissa11  = rshift( mantissa11, -( 19 + exponent));
        //  Set denormalized exponent.
        exponent = 0;
    }

    //  Assemble the float11 value.
    uZ.ui = (uint16_t(exponent & 0x3f) << 6) | mantissa11;
    return uZ.f;
}
