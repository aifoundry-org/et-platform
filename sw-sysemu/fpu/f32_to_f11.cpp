#include "fpu_types.h"
#include "fpu_casts.h"
#include "softfloat/internals.h"
#include "softfloat/specialize.h"


// safe right shifts ( shifting by more than 32 bits returns 0)
template<typename T> T rshift(T v, unsigned int s) {
  if (s >= sizeof(T)*8)  return 0;
  else return v >> s;
}


float11_t f32_to_f11(float32_t val)
{
    uint8_t  sign;
    uint32_t mantissa32;
    uint32_t mantissa11;
    int32_t  exponent;
    uint32_t inputAux;
    bool denorm;
    bool nan;
    bool zero;
    bool infinity;

    inputAux = fpu::UI32(val);

    //  Disassemble float32_t value into sign, exponent and mantissa.

    //  Extract sign.
    sign = ((inputAux & 0x80000000) == 0) ? 0 : 1;

    //  Extract exponent.
    exponent = (inputAux >> 23) & 0xff;

    //  Extract mantissa.
    mantissa32 = inputAux & 0x007fffff;

    //  Compute flags.
    denorm = (exponent == 0) && (mantissa32 != 0);
    zero = (exponent == 0) && (mantissa32 == 0);
    nan = (exponent == 0xff) && (mantissa32 != 0);
    infinity = (exponent == 0xff) && (mantissa32 == 0);

    //  Flush float32_t denorms to 0
    if (denorm || zero || (sign != 0)) {
        return fpu::F11(0);
    }
    if (nan) {
        if (softfloat_isSigNaNF32UI(inputAux))
            softfloat_raiseFlags(softfloat_flag_invalid);
        return fpu::F11(0x07e0);
    }
    if (infinity) {
        return fpu::F11(0x07c0);
    }
    //  Flush numbers with an exponent not representable in float11 to infinite.
    if (exponent > (127 + 15)) {
        softfloat_raiseFlags(softfloat_flag_overflow);
        return fpu::F11(0x07bf);
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
    return fpu::F11((uint16_t(exponent & 0x3f) << 6) | mantissa11);
}
