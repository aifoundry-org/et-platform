#include <emu.h>
#include <cvt.h>
#include <math.h>

// safe right shifts ( shifting by more than 32 bits returns 0)
template<typename T> T rshift(T v, int s) {
  if (s >= sizeof(T)*8)  return 0;
  else return v >> s;
}


float32 float16tofloat32(uint32 val) {
    uint8  sign;
    uint32 mantissa;
    int32  exponent;
    uint32 output;
    uint32 normbits;
    bool denorm;
    bool infinite;
    bool nan;
    bool zero;

    //  Disassemble float16 value in sign, exponent and mantissa.
    //  Read float16 sign.
    sign = ((val & 0x8000) == 0) ? 0 : 1;

    //  Read the float16 exponent.
    exponent = ((val & 0x7c00) >> 10);

    //  Read the float16 mantissa (explicit bits).
    mantissa = (val & 0x03ff);

    //  Compute flags.
    denorm = (exponent == 0) && (mantissa != 0);
    zero = (exponent == 0) && (mantissa == 0);
    infinite = (exponent == 0x1f) && (mantissa == 0);
    nan = (exponent == 0x1f) && (mantissa != 0);

    //  Return zeros.
    if (zero)
       return (sign == 0) ? 0.0f : -0.0f;

    //  Convert exponent and mantissa to float32.  Convert from excess 15 and convert to excess 127.
    if (infinite || nan)
        exponent = 0xff;
    else
        exponent = denorm ? (-14 + 127) : (exponent - 15) + 127;

    // for nan, return sign is zero because it returns a canonical  nan
    if (nan)
      sign = 0;

    //  Convert mantissa to float32.  Mantissa goes from 10+1 bits to 23+1 bits.
    if (infinite)
        mantissa = mantissa;
    else if (nan)
        mantissa = 1 << 22;
    else
        mantissa = (mantissa << 13);

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
        else if ((mantissa & 0x00010000) != 0)
            normbits = 7;
        else if ((mantissa & 0x00008000) != 0)
            normbits = 8;
        else if ((mantissa & 0x00004000) != 0)
            normbits = 9;
        else if ((mantissa & 0x00002000) != 0)
            normbits = 10;
        //else
            //panic("GPUMath", "convertFP16ToFP32", "Error normalizing denormalized float16 mantissa.");

        //  Decrease exponent 1 bit.
        exponent = exponent - normbits;

        //  Shift exponent 1 bit and remove implicit bit;
        mantissa = ((mantissa << normbits) & 0x007fffff);
    }

    //  Assemble the float32 value with the obtained sign, exponent and mantissa.
    output = (sign << 31) | (uint32(exponent & 0xff) << 23) | mantissa;

    return *((float32 *) &output);
}

float32 float11tofloat32(uint32 val) {
    uint8  sign;
    uint32 mantissa;
    int32  exponent;
    uint32 output;
    uint32 normbits;
    bool denorm;
    bool infinite;
    bool nan;
    bool zero;

    //  Disassemble float11 value in sign, exponent and mantissa.
    //  Read float11 sign.
    sign = 0;

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
       return 0.0f;

    //  Convert exponent and mantissa to float32.  Convert from excess 15 and convert to excess 127.
    if (infinite || nan)
        exponent = 0xff;
    else
        exponent = denorm ? (-14 + 127) : (exponent - 15) + 127;

    //  Convert mantissa to float32.  Mantissa goes from 6+1 bits to 23+1 bits.
    if (infinite || nan)
        mantissa = mantissa;
    else
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

    //  Assemble the float32 value with the obtained sign, exponent and mantissa.
    output = (uint32(exponent & 0xff) << 23) | mantissa;

    return *((float32 *) &output);
}

float32 float10tofloat32(uint32 val) {
    uint8  sign;
    uint32 mantissa;
    int32  exponent;
    uint32 output;
    uint32 normbits;
    bool denorm;
    bool infinite;
    bool nan;
    bool zero;

    //  Disassemble float10 value in sign, exponent and mantissa.
    //  Read float11 sign.
    sign = 0;

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
       return 0.0f;

    //  Convert exponent and mantissa to float32.  Convert from excess 15 and convert to excess 127.
    if (infinite || nan)
        exponent = 0xff;
    else
        exponent = denorm ? (-14 + 127) : (exponent - 15) + 127;

    //  Convert mantissa to float32.  Mantissa goes from 5+1 bits to 23+1 bits.
    if (infinite || nan)
        mantissa = mantissa;
    else
        mantissa = (mantissa << 18);

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

    //  Assemble the float32 value with the obtained sign, exponent and mantissa.
    output = (uint32(exponent & 0xff) << 23) | mantissa;

    return *((float32 *) &output);
}

uint32 float32tofloat16(float32 val) {
    uint8  sign;
    uint32 mantissa32;
    uint32 mantissa16;
    int32  exponent;
    uint32 inputAux;
    bool denorm;
    bool infinite;
    bool nan;
    bool zero;

    inputAux = *((uint32 *) &val);

    //  Disassemble float32 value into sign, exponent and mantissa.

    //  Extract sign.
    sign = ((inputAux & 0x80000000) == 0) ? 0 : 1;

    //  Extract exponent.
    exponent = (inputAux >> 23) & 0xff;

    //  Extract mantissa.
    mantissa32 = inputAux & 0x007fffff;
   
    //  Compute flags.
    denorm = (exponent == 0) && (mantissa32 != 0);
    zero = (exponent == 0) && (mantissa32 == 0);
    infinite = (exponent == 0xff) && (mantissa32 == 0);
    nan = (exponent == 0xff) && (mantissa32 != 0);

    //  Flush float32 denorms to 0
    if (denorm || zero)
        return (sign == 0) ? 0x0000 : 0x8000;

    if (nan)
        return 0x7e00;
    //  Flush numbers with an exponent not representable in float16 to infinite.
    else if (exponent > (127 + 15))
        return ((sign == 0) ? 0x7c00 : 0xfc00);
    else
    {
        //  Convert exponent to float16.  Excess 127 to excess 15.
        exponent = exponent - 127 + 15;

        if (exponent > 0)
        {
            //  Convert mantissa from float32 to float16.  Mantissa 23+1 to mantissa 10+1.
            mantissa16 = (mantissa32 >> 13) & 0x03ff;

            // apply rounding
            mantissa16 += (mantissa32 >> 12) & 0x1; // Round to nearest, ties to Max Magnitude
            //mantissa16 += ((mantissa32>>12) & 0x1) &
            //              (((mantissa32 & 0x2FFF) != 0) ? 1 : 0); // Round to nearest, ties to even

            // increment exponent if there was overflow
            if ( (mantissa16 >> 10) != 0 ) {
              exponent ++;
              mantissa16 = 0;
            }
        }
        //  Check for denormalized float16 number.
        else
        {
            //  Add implicit float32 bit to the mantissa.
            mantissa32 = mantissa32 + 0x800000;

            //  Convert mantissa from float32 to float16.  Mantissa 23+1 to mantissa 10+1.
            mantissa16 = (mantissa32 >> 13) & 0x07ff;

            //  Denormalize mantissa.
            mantissa16 = rshift(mantissa16, (-exponent + 1));

            // apply rounding
             mantissa16 += rshift(mantissa32, (13-exponent)) & 0x1; // Round to nearest, ties to Max Magnitude
             //mantissa16 += (rshift(mantissa32, (13-exponent)) & 0x1) &
             //((((rshift(mantissa32,(14-exponent)) & 0x1) != 0) && ((mantissa32 & (1<<(13-exponent)-1)) != 0)) ? 1 : 0); // Round to nearest, ties to even

            // increment exponent if there was overflow
            if ( (mantissa16 >> 11) != 0 ) {
              exponent ++;
              mantissa16 = mantissa16 >> 1;
            }
            if (exponent < -24)
              mantissa16  = rshift( mantissa16, -( 24 + exponent));
            //  Set denormalized exponent.
            exponent = 0;
        }

        //  Assemble the float16 value.
        return (sign << 15) | (uint32(exponent & 0x3f) << 10) | mantissa16;
    }
}

uint32 float32tofloat11(float32 val) {
    uint8  sign;
    uint32 mantissa32;
    uint32 mantissa11;
    int32  exponent;
    uint32 inputAux;
    bool denorm;
    bool infinite;
    bool nan;
    bool zero;

    inputAux = *((uint32 *) &val);

    //  Disassemble float32 value into sign, exponent and mantissa.

    //  Extract sign.
    sign = ((inputAux & 0x80000000) == 0) ? 0 : 1;

    //  Extract exponent.
    exponent = (inputAux >> 23) & 0xff;

    //  Extract mantissa.
    mantissa32 = inputAux & 0x007fffff;

    //  Compute flags.
    denorm = (exponent == 0) && (mantissa32 != 0);
    zero = (exponent == 0) && (mantissa32 == 0);
    infinite = (exponent == 0xff) && (mantissa32 == 0);
    nan = (exponent == 0xff) && (mantissa32 != 0);

    //  Flush float32 denorms to 0
    if (denorm || zero || (sign != 0))
        return 0x0000;

    if (nan)
        return 0x07e0;
    //  Flush numbers with an exponent not representable in float11 to infinite.
    else if (exponent > (127 + 15))
        return 0x07c0;
    else
    {
        //  Convert exponent to float11.  Excess 127 to excess 15.
        exponent = exponent - 127 + 15;

        if (exponent > 0)
        {
            //  Convert mantissa from float32 to float11.  Mantissa 23+1 to mantissa 6+1.
            mantissa11 = (mantissa32 >> 17) & 0x003f;

            // apply rounding
            mantissa11 += (mantissa32 >> 16) & 0x1; // Round to nearest, ties to Max Magnitude
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
            //  Add implicit float32 bit to the mantissa.
            mantissa32 = mantissa32 + 0x800000;

            //  Convert mantissa from float32 to float16.  Mantissa 23+1 to mantissa 10+1.
            mantissa11 = (mantissa32 >> 17) & 0x07f;

            //  Denormalize mantissa.
            mantissa11 = rshift(mantissa11, -exponent + 1);

            // apply rounding
            mantissa11 += rshift (mantissa32, (17-exponent)) & 0x1; // Round to nearest, ties to Max Magnitude
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
        return (uint32(exponent & 0x3f) << 6) | mantissa11;
    }
}

uint32 float32tofloat10(float32 val) {
    uint8  sign;
    uint32 mantissa32;
    uint32 mantissa10;
    int32  exponent;
    uint32 inputAux;
    bool denorm;
    bool infinite;
    bool nan;
    bool zero;

    inputAux = *((uint32 *) &val);

    //  Disassemble float32 value into sign, exponent and mantissa.

    //  Extract sign.
    sign = ((inputAux & 0x80000000) == 0) ? 0 : 1;

    //  Extract exponent.
    exponent = (inputAux >> 23) & 0xff;

    //  Extract mantissa.
    mantissa32 = inputAux & 0x007fffff;

    //  Compute flags.
    denorm = (exponent == 0) && (mantissa32 != 0);
    zero = (exponent == 0) && (mantissa32 == 0);
    infinite = (exponent == 0xff) && (mantissa32 == 0);
    nan = (exponent == 0xff) && (mantissa32 != 0);

    //  Flush float32 denorms to 0
    if (denorm || zero || (sign != 0))
        return 0x0000;

    if (nan)
        return 0x03f0;
    //  Flush numbers with an exponent not representable in float10 to infinite.
    else if (exponent > (127 + 15))
        return 0x03e0;
    else
    {
        //  Convert exponent to float10.  Excess 127 to excess 15.
        exponent = exponent - 127 + 15;

        if (exponent > 0)
        {
            //  Convert mantissa from float32 to float10.  Mantissa 23+1 to mantissa 5+1.
            mantissa10 = (mantissa32 >> 18) & 0x001f;

            // apply rounding
            mantissa10 += (mantissa32 >> 17) & 0x1; // Round to nearest, ties to Max Magnitude
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
            //  Add implicit float32 bit to the mantissa.
            mantissa32 = mantissa32 + 0x800000;

            //  Convert mantissa from float32 to float16.  Mantissa 23+1 to mantissa 5+1.
            mantissa10 = (mantissa32 >> 18) & 0x003f;

            //  Denormalize mantissa.
            mantissa10 = rshift(mantissa10, (-exponent + 1));

            // apply rounding
            mantissa10 += rshift(mantissa32 , (18-exponent)) & 0x1; // Round to nearest, ties to Max Magnitude
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
        return (uint32(exponent & 0x1f) << 5) | mantissa10;
    }
}

float32 unorm24tofloat32(uint32 val) {
    uint32 maxrange = (1 << 24) - 1;
    return ( (float32)(val & 0xFFFFFF) / (float32)maxrange);
}

float32 unorm16tofloat32(uint32 val) {
    uint32 maxrange = (1 << 16) - 1;
    return ((float32)(val & 0xFFFF) / (float32)maxrange);
}

float32 unorm10tofloat32(uint32 val){
    uint32 maxrange = (1 << 10) - 1;
    return ((float32)(val & 0x3FF) / (float32)maxrange);
};

float32 unorm8tofloat32(uint32 val) {
    uint32 maxrange = (1 << 8 ) - 1;
    return ( (float32)(val & 0xFF) / (float32)maxrange);
}

float32 unorm2tofloat32(uint32 val){
    uint32 maxrange = (1 << 2) - 1;
    return ((float32)(val & 0x3) / (float32)maxrange);
};

float32 snorm24tofloat32(uint32 val) {
    if (val == (1 << 23)) val = (1 << 23) + 1;
    float32 sign = ((val & 0x00800000) == 0) ? 1 : -1;
    float32 value = ((sign < 0) ? (~val + 1) : val) & 0x007fffff;
    float32 maxrange = float32((1 << 23) - 1);
    return sign * (value / maxrange);
}

float32 snorm16tofloat32(uint32 val) {
    if (val == (1 << 15)) val = (1 << 15) + 1;
    float32 sign = ((val & 0x00008000) == 0) ? 1 : -1;
    float32 value = ((sign < 0) ? (~val + 1) : val) & 0x00007fff;
    float32 maxrange = float32((1 << 15) - 1);
    return sign * (value / maxrange);
}

float32 snorm10tofloat32(uint32 val) {
    if (val == (1 << 9)) val = (1 << 9) + 1;
    float32 sign = ((val & 0x000200) == 0) ? 1 : -1;
    float32 value = ((sign < 0) ? (~val + 1) : val) & 0x0000001ff;
    float32 maxrange = float32((1 << 9) - 1);
    return sign * (value / maxrange);
}

float32 snorm8tofloat32(uint32 val) {
    if (val == (1 << 7)) val = (1 << 7) + 1;
    float32 sign = ((val & 0x000080) == 0) ? 1 : -1;
    float32 value = ((sign < 0) ? (~val + 1) : val) & 0x0000007f;
    float32 maxrange = float32((1 << 7) - 1);
    return sign * (value / maxrange);
}

float32 snorm2tofloat32(uint32 val) {
    if (val == (1 << 1)) val = (1 << 1) + 1;
    float32 sign = ((val & 0x00000002) == 0) ? 1 : -1;
    float32 value = ((sign < 0) ? (~val + 1) : val) & 0x00000001;
    float32 maxrange = float32((1 << 1) - 1);
    return sign * (value / maxrange);
}

uint32 float32tounorm24(float32 val) {
    float64 delta;
    float64 ratio;

    if((val >= 1.0) || isnan(val))
        return 0x00ffffff;
    else if (val <= 0.0)
        return 0x00000000;
    else {
        delta = (1 << 24) - 1;
        ratio = val * delta;
        return (uint32(ratio + 0.5f));
    }

}

uint32 float32tounorm16(float32 val) {
    float64 delta;
    float64 ratio;

    if((val >= 1.0) || isnan(val))
        return 0x0000ffff;
    else if (val <= 0.0)
        return 0x00000000;
    else {
        delta = (1 << 16) - 1;
        ratio = val * delta;
        return (uint32(ratio + 0.5f));
    }

}

uint32 float32tounorm10(float32 val) {
    float64 delta;
    float64 ratio;

    if((val >= 1.0) || isnan(val))
        return 0x000003ff;
    else if (val <= 0.0)
        return 0x00000000;
    else {
        delta = (1 << 10) - 1;
        ratio = val * delta;
        return (uint32(ratio + 0.5f));
    }

}

uint32 float32tounorm8(float32 val) {
    float64 delta;
    float64 ratio;

    if((val >= 1.0) || isnan(val))
        return 0x000000ff;
    else if (val <= 0.0)
        return 0x00000000;
    else {
        delta = (1 << 8) - 1;
        ratio = val * delta;
        return (uint32(ratio + 0.5f));
    }

}

uint32 float32tounorm2(float32 val) {
    float64 delta;
    float64 ratio;

    if((val >= 1.0) || isnan(val))
        return 0x00000003;
    else if (val <= 0.0)
        return 0x00000000;
    else {
        delta = (1 << 2) - 1;
        ratio = val * delta;
        return (uint32(ratio + 0.5f));
    }

}

uint32 float32tosnorm24(float32 val) {
    if((val >= 1.0) || isnan(val))
        return 0x007fffff;
    else if (val <= -1.0)
        return 0x00800001;
    else
    {
        float32 int_val = round(val * ((1 << 23) - 1));
        int32 res = int32(int_val);
        return uint32(res) & 0x00ffffff;
    }

}

uint32 float32tosnorm16(float32 val) {
    if((val >= 1.0) || isnan(val))
        return 0x00007fff;
    else if (val <= -1.0)
        return 0x00008001;
    else
    {
        float32 int_val = round(val * ((1 << 15) - 1));
        int32 res = int32(int_val);
        return uint32(res) & 0x00ffff;
    }
}

uint32 float32tosnorm8(float32 val)
{
    if((val >= 1.0) || isnan(val))
        return 0x0000007f;
    else if (val <= -1.0)
        return 0x00000081;
    else
    {
        float32 int_val = round(val * ((1 << 7) - 1));
        int32 res = int32(int_val);
        return uint32(res) & 0x00ff;
    }
}
