#include <cmath>
#include "cvt.h"
#include "fpu_casts.h"
#include "softfloat/softfloat.h"
#include "softfloat/internals.h"
#include "softfloat/specialize.h"

// safe right shifts ( shifting by more than 32 bits returns 0)
template<typename T> T rshift(T v, unsigned int s) {
  if (s >= sizeof(T)*8)  return 0;
  else return v >> s;
}

// FIXME: These conversion functions should set arithmetic flags

float32_t float11tofloat32(uint16_t val)
{
    uint32_t mantissa;
    int32_t  exponent;
    uint32_t output;
    uint32_t normbits = 0;
    bool denorm;
    bool infinite;
    bool nan;
    bool zero;

    //  Read the float11 exponent.
    exponent = ((val & 0x07c0) >> 6);

    //  Read the float11 mantissa (explicit bits).
    mantissa = (val & 0x003f);

    //  Compute flags.
    denorm = (exponent == 0) && (mantissa != 0);
    zero = (exponent == 0) && (mantissa == 0);
    infinite = (exponent == 0x1f) && (mantissa == 0);
    nan = (exponent == 0x1f) && (mantissa != 0);

    //  Return zeros.
    if (zero)
       return fpu::F32(0);

    //  Convert exponent and mantissa to float32_t.  Convert from excess 15 and convert to excess 127.
    if (infinite || nan)
        exponent = 0xff;
    else
        exponent = denorm ? (-14 + 127) : (exponent - 15) + 127;

    //  Convert mantissa to float32_t.  Mantissa goes from 6+1 bits to 23+1 bits.
    if (nan)
        return fpu::F32(defaultNaNF32UI);
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
    output = (uint32_t(exponent & 0xff) << 23) | mantissa;

    return fpu::F32(output);
}

float32_t float10tofloat32(uint16_t val)
{
    uint32_t mantissa;
    int32_t  exponent;
    uint32_t output;
    uint32_t normbits = 0;
    bool denorm;
    bool infinite;
    bool nan;
    bool zero;

    //  Read the float10 exponent.
    exponent = ((val & 0x03e0) >> 5);

    //  Read the float10 mantissa (explicit bits).
    mantissa = (val & 0x001f);

    //  Compute flags.
    denorm = (exponent == 0) && (mantissa != 0);
    zero = (exponent == 0) && (mantissa == 0);
    infinite = (exponent == 0x1f) && (mantissa == 0);
    nan = (exponent == 0x1f) && (mantissa != 0);

    //  Return zeros.
    if (zero)
       return fpu::F32(0);

    //  Convert exponent and mantissa to float32_t.  Convert from excess 15 and convert to excess 127.
    if (infinite || nan)
        exponent = 0xff;
    else
        exponent = denorm ? (-14 + 127) : (exponent - 15) + 127;

    //  Convert mantissa to float32_t.  Mantissa goes from 5+1 bits to 23+1 bits.
    if (nan)
        return fpu::F32(defaultNaNF32UI);
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
        //else
            //panic("GPUMath", "convertFP16ToFP32", "Error normalizing denormalized float16 mantissa.");

        //  Decrease exponent 1 bit.
        exponent = exponent - normbits;

        //  Shift exponent 1 bit and remove implicit bit;
        mantissa = ((mantissa << normbits) & 0x007fffff);
    }

    //  Assemble the float32_t value with the obtained sign, exponent and mantissa.
    output = (uint32_t(exponent & 0xff) << 23) | mantissa;

    return fpu::F32(output);
}

uint16_t float32tofloat11(float32_t val)
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
    if (denorm || zero || (sign != 0))
        return 0x0000;

    if (nan)
        return 0x07e0;
    else if(infinity)
        return 0x07c0;
    //  Flush numbers with an exponent not representable in float11 to infinite.
    else if (exponent > (127 + 15))
        return 0x07bf;
    else
    {
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
        return (uint16_t(exponent & 0x3f) << 6) | mantissa11;
    }
}

uint16_t float32tofloat10(float32_t val)
{
    uint8_t  sign;
    uint32_t mantissa32;
    uint32_t mantissa10;
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
    if (denorm || zero || (sign != 0))
        return 0x0000;

    if (nan)
        return 0x03f0;
    //  Flush numbers with an exponent not representable in float10 to infinite.
    else if (infinity)
        return 0x03e0;
    else if (exponent > (127 + 15))
        return 0x03df;
    else
    {
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
        return (uint16_t(exponent & 0x1f) << 5) | mantissa10;
    }
}

float32_t unorm24tofloat32(uint32_t val)
{
    uint32_t maxrange = (1 << 24) - 1;
    float res = float(val & 0xFFFFFF) / float(maxrange);
    return fpu::F2F32(res);
}

float32_t unorm16tofloat32(uint16_t val)
{
    uint32_t maxrange = (1 << 16) - 1;
    float res = float(val & 0xFFFF) / float(maxrange);
    return fpu::F2F32(res);
}

float32_t unorm10tofloat32(uint16_t val)
{
    uint32_t maxrange = (1 << 10) - 1;
    float res = float(val & 0x3FF) / float(maxrange);
    return fpu::F2F32(res);
};

float32_t unorm8tofloat32(uint8_t val)
{
    uint32_t maxrange = (1 << 8 ) - 1;
    float res = float(val & 0xFF) / float(maxrange);
    return fpu::F2F32(res);
}

float32_t unorm2tofloat32(uint8_t val)
{
    uint32_t maxrange = (1 << 2) - 1;
    float res = float(val & 0x3) / float(maxrange);
    return fpu::F2F32(res);
};

float32_t snorm24tofloat32(uint32_t val)
{
    if (val == (1 << 23)) val = (1 << 23) + 1;
    int sign = ((val & 0x00800000) == 0) ? 1 : -1;
    uint32_t value = ((sign < 0) ? (~val + 1) : val) & 0x007fffff;
    uint32_t maxrange = (1 << 23) - 1;
    float res = float(sign) * (float(value) / float(maxrange));
    return fpu::F2F32(res);
}

float32_t snorm16tofloat32(uint16_t val)
{
    if (val == (1 << 15)) val = (1 << 15) + 1;
    int sign = ((val & 0x00008000) == 0) ? 1 : -1;
    uint32_t value = ((sign < 0) ? (~val + 1) : val) & 0x00007fff;
    uint32_t maxrange = (1 << 15) - 1;
    float res = float(sign) * (float(value) / float(maxrange));
    return fpu::F2F32(res);
}

float32_t snorm10tofloat32(uint16_t val)
{
    if (val == (1 << 9)) val = (1 << 9) + 1;
    int sign = ((val & 0x000200) == 0) ? 1 : -1;
    uint32_t value = ((sign < 0) ? (~val + 1) : val) & 0x0000001ff;
    uint32_t maxrange = (1 << 9) - 1;
    float res = float(sign) * (float(value) / float(maxrange));
    return fpu::F2F32(res);
}

float32_t snorm8tofloat32(uint8_t val)
{
    if (val == (1 << 7)) val = (1 << 7) + 1;
    int sign = ((val & 0x000080) == 0) ? 1 : -1;
    uint32_t value = ((sign < 0) ? (~val + 1) : val) & 0x0000007f;
    uint32_t maxrange = (1 << 7) - 1;
    float res = float(sign) * (float(value) / float(maxrange));
    return fpu::F2F32(res);
}

float32_t snorm2tofloat32(uint8_t val)
{
    if (val == (1 << 1)) val = (1 << 1) + 1;
    int sign = ((val & 0x00000002) == 0) ? 1 : -1;
    uint32_t value = ((sign < 0) ? (~val + 1) : val) & 0x00000001;
    uint32_t maxrange = (1 << 1) - 1;
    float res = float(sign) * (float(value) / float(maxrange));
    return fpu::F2F32(res);
}

uint32_t float32tounorm24(float32_t val)
{
    int32_t val_i = int32_t(val.v);
    if ((val_i >= 0x3f800000) ||  isNaNF32UI(val.v))
        return 0x00ffffff;
    else if (val_i <= 0)
        return 0x00000000;
    else {
        uint32_t delta = (1 << 24) - 1;
        double ratio = double(fpu::FLT(val)) * double(delta);
        return uint32_t(ratio + 0.5f);
    }
}

uint16_t float32tounorm16(float32_t val)
{
    int32_t val_i = int32_t(val.v);
    if ((val_i >= 0x3f800000) ||  isNaNF32UI(val.v))
        return 0xffff;
    else if (val_i <= 0)
        return 0x0000;
    else {
        uint32_t delta = (1 << 16) - 1;
        double ratio = double(fpu::FLT(val)) * double(delta);
        return uint16_t(ratio + 0.5f);
    }
}

uint16_t float32tounorm10(float32_t val)
{
    int32_t val_i = int32_t(val.v);
    if ((val_i >= 0x3f800000) ||  isNaNF32UI(val.v))
        return 0x03ff;
    else if (val_i <= 0)
        return 0x0000;
    else {
        uint32_t delta = (1 << 10) - 1;
        double ratio = double(fpu::FLT(val)) * double(delta);
        return uint16_t(ratio + 0.5f);
    }
}

uint8_t float32tounorm8(float32_t val)
{
    int32_t val_i = int32_t(val.v);
    if ((val_i >= 0x3f800000) ||  isNaNF32UI(val.v))
        return 0xff;
    else if (val_i <= 0)
        return 0x00;
    else {
        uint32_t delta = (1 << 8) - 1;
        double ratio = double(fpu::FLT(val)) * double(delta);
        return uint8_t(ratio + 0.5f);
    }
}

uint8_t float32tounorm2(float32_t val)
{
    int32_t val_i = int32_t(val.v);
    if ((val_i >= 0x3f800000) ||  isNaNF32UI(val.v))
        return 0x03;
    else if (val_i <= 0)
        return 0x00;
    else {
        uint32_t delta = (1 << 2) - 1;
        double ratio = double(fpu::FLT(val)) * double(delta);
        return uint8_t(ratio + 0.5f);
    }
}

uint32_t float32tosnorm24(float32_t val)
{
    int32_t val_i = int32_t(val.v);
    if ((val_i >= 0x3f800000) ||  isNaNF32UI(val.v))
        return 0x007fffff;
    else if (val.v >= 0xbf800000)
        return 0x00800001;
    else {
        float int_val = round(fpu::FLT(val) * float((1 << 23) - 1));
        int32_t res = int32_t(int_val);
        return uint32_t(res & 0x00ffffff);
    }
}

uint16_t float32tosnorm16(float32_t val)
{
    int32_t val_i = int32_t(val.v);
    if ((val_i >= 0x3f800000) ||  isNaNF32UI(val.v))
        return 0x7fff;
    else if (val.v >= 0xbf800000)
        return 0x8001;
    else {
        float int_val = round(fpu::FLT(val) * float((1 << 15) - 1));
        int32_t res = int32_t(int_val);
        return uint16_t(res & 0x0000ffff);
    }
}

uint8_t float32tosnorm8(float32_t val)
{
    int32_t val_i = int32_t(val.v);
    if ((val_i >= 0x3f800000) ||  isNaNF32UI(val.v))
        return 0x7f;
    else if (val.v >= 0xbf800000)
        return 0x81;
    else {
        float int_val = round(fpu::FLT(val) * float((1 << 7) - 1));
        int32_t res = int32_t(int_val);
        return uint8_t(res & 0x000000ff);
    }
}
