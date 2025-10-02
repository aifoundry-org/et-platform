/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#include <cmath>

#include "fpu_types.h"
#include "fpu_casts.h"
#include "softfloat/platform.h"
#include "softfloat/internals.h"
#include "softfloat/specialize.h"

#include "trcp.h"
#include "trsqrt.h"
#include "texp.h"
#include "tsin.h"

#define GET(in, a, b) ((in >> b) % (((uint64_t)1) << (a-b+1)))
#define MERGE(s, e, m) (((s % 2) << 31) + ((e % (1 << 8)) << 23) + (m % (1 << 23)))

#define zeroF32UI          0x00000000
#define oneF32UI           0x3F800000
#define infinityF32UI      0x7F800000
#define minusZeroF32UI     0x80000000
#define minusOneF32UI      0xBF800000
#define minusInfinityF32UI 0xFF800000


static uint64_t trans_fma(uint32_t val, uint32_t c2, uint32_t c1, uint32_t c0)
{
    uint32_t shifted_c2 = GET(c2,  10, 0) << 21;
    uint32_t shifted_c1 = GET(c1,  18, 0) << 13;
    uint32_t shifted_x  = GET(val, 16, 0) << 6;

    //printf("Original C2: %04x\tShifted C2: %08x\n", c2, shifted_c2);
    //printf("Original C1: %04x\tShifted C1: %08x\n", c1, shifted_c1);
    //printf("Original X: %08x\tShifted X: %08x\n", val, shifted_x);

    uint64_t mul1 = ((uint64_t)shifted_c2)*((uint64_t) shifted_x);
    uint32_t shifted_mul1 = GET(mul1, 63, 32);

    //printf("Original mul: %016lx\tShifted mul: %08x\n", mul1, shifted_mul1);

    uint32_t fma1 = shifted_mul1 + shifted_c1 +1;

    //printf("EMU FMA1: %08x\n", fma1);

    uint32_t shifted_fma1 = fma1;
    uint64_t shifted_c0 =((uint64_t) GET(c0, 27,0)) << 32;

    //printf("Original FMA1: %08x\tShifted FMA1: %08x\n", fma1, shifted_fma1);
    //printf("Original C0: %04x\tShifted C0: %016lx\n", c0, shifted_c0);
    //printf("Original X: %08x\tShifted X: %08x\n", val, shifted_x);

    uint64_t mul2 = (((uint64_t) 0xffffffff00000000) + (uint64_t)shifted_fma1 )*((uint64_t) shifted_x);

    uint64_t fma_result = mul2 + shifted_c0;

    //printf("FMA RESULT: %016lx\n", fma_result);

    return fma_result;
}


float32_t f32_rcp(float32_t a)
{
    ui32_f32 uZ;
    uint32_t val = fpu::UI32(a);
#ifdef SOFTFLOAT_DENORMALS_TO_ZERO
    if (isSubnormalF32UI(val)) {
        softfloat_raiseFlags(softfloat_flag_denormal);
        val = softfloat_zeroExpSigF32UI(val);
    }
#endif
    switch (val) {
      case minusInfinityF32UI:
          uZ.ui = minusZeroF32UI;
          return uZ.f;
      case infinityF32UI:
          uZ.ui = zeroF32UI;
          return uZ.f;
      case minusZeroF32UI:
          softfloat_raiseFlags(softfloat_flag_infinite);
          uZ.ui = minusInfinityF32UI;
          return uZ.f;
      case zeroF32UI:
          softfloat_raiseFlags(softfloat_flag_infinite);
          uZ.ui = infinityF32UI;
          return uZ.f;
      default:
          break;
    }
    if (isNaNF32UI(val)) {
        if (softfloat_isSigNaNF32UI(val))
            softfloat_raiseFlags(softfloat_flag_invalid);
        uZ.ui = defaultNaNF32UI;
        return uZ.f;
    }
    if((val & 0x7fffffff) > 0x7e800000) {
        uZ.ui = (val & 0x80000000) ? minusZeroF32UI : zeroF32UI;
        return uZ.f;
    }

    uint32_t idx = GET(val, 22, 16); // input bits [22:16]

    uint32_t c2 = trcp[idx][0] << 1;
    uint32_t c1 = 0x70000 + trcp[idx][1];
    uint32_t c0 = 0x02000000 + trcp[idx][2];

    uint32_t masked_val = val &  0xffff;

    uint64_t fma_result = trans_fma(masked_val, c2, c1, c0);
    uint64_t fma_rounded = fma_result + (((uint64_t)1) << 33);

    uint32_t fraction_res = GET(fma_rounded, 57, 34);

    uint16_t exp = GET(val, 30, 23);
    uint32_t mantissa  = GET(val, 22, 0);

    uint16_t res_exp = 254 - exp - (mantissa != 0);

    uZ.ui = MERGE(GET(val, 31, 31), res_exp, fraction_res);
    //printf("EMU FMA2: %08x\n", result);
#ifdef SOFTFLOAT_DENORMALS_TO_ZERO
    if (isSubnormalF32UI(uZ.ui)) {
        softfloat_raiseFlags(
            softfloat_flag_underflow | softfloat_flag_inexact);
        uZ.ui = softfloat_zeroExpSigF32UI(uZ.ui);
    }
#endif
    return uZ.f;
}


float32_t f32_rsqrt(float32_t a)
{
    ui32_f32 uZ;
    uint32_t val = fpu::UI32(a);
#ifdef SOFTFLOAT_DENORMALS_TO_ZERO
    if (isSubnormalF32UI(val)) {
        softfloat_raiseFlags(softfloat_flag_denormal);
        val = softfloat_zeroExpSigF32UI(val);
    }
#endif
    switch (val) {
      case minusZeroF32UI:
          softfloat_raiseFlags(softfloat_flag_infinite);
          uZ.ui = minusInfinityF32UI;
          return uZ.f;
      case zeroF32UI:
          softfloat_raiseFlags(softfloat_flag_infinite);
          uZ.ui = infinityF32UI;
          return uZ.f;
      case infinityF32UI:
          uZ.ui = zeroF32UI;
          return uZ.f;
      default:
          break;
    }
    if (isNaNF32UI(val)) {
        if (softfloat_isSigNaNF32UI(val))
            softfloat_raiseFlags(softfloat_flag_invalid);
        uZ.ui = defaultNaNF32UI;
        return uZ.f;
    }
    if (val & 0x80000000) {
        softfloat_raiseFlags(softfloat_flag_invalid);
        uZ.ui = defaultNaNF32UI;
        return uZ.f;
    }

    uint16_t idx = GET(val,23, 16);

    uint32_t c2 = trsqrt[idx][0] << 2;
    uint32_t c1 = 0x70000 + (trsqrt[idx][1] << 1);
    uint32_t c0 = 0x4000000 + (trsqrt[idx][2] << GET(val, 23, 23));

    uint32_t masked_val = 0xffff & val;

    uint64_t fma_result = trans_fma(masked_val, c2, c1, c0);
    uint64_t fma_rounded = fma_result + (((uint64_t)1) << 34);

    uint32_t mantissa  = GET(val, 22, 0);
    uint32_t fraction_res = GET(fma_rounded,57, 35);

    uint16_t exponent = GET(val, 30, 23) -1;
    uint16_t exponent_shifted = ((exponent >> 1) ^ 0x40) + (exponent & 0x80);
    uint16_t exponent_res = (~exponent_shifted)-1 - ((mantissa != 0) || (GET(val, 23, 23) == 0));

    //printf("E: 0x%04x\tES: 0x%04x\tER: 0x%04x\tM: %04x\n", exponent, exponent_shifted, exponent_res, mantissa);

    uZ.ui = MERGE(0, exponent_res, fraction_res);
#ifdef SOFTFLOAT_DENORMALS_TO_ZERO
    if (isSubnormalF32UI(uZ.ui)) {
        softfloat_raiseFlags(
            softfloat_flag_underflow | softfloat_flag_inexact);
        uZ.ui = softfloat_zeroExpSigF32UI(uZ.ui);
    }
#endif
    return uZ.f;
}


float32_t f32_exp2(float32_t a)
{
    ui32_f32 uZ;
    uint32_t val = fpu::UI32(a);
#ifdef SOFTFLOAT_DENORMALS_TO_ZERO
    if (isSubnormalF32UI(val)) {
        softfloat_raiseFlags(softfloat_flag_denormal);
        val = softfloat_zeroExpSigF32UI(val);
    }
#endif

    if (isNaNF32UI(val)) {
        if (softfloat_isSigNaNF32UI(val))
            softfloat_raiseFlags(softfloat_flag_invalid);
        uZ.ui = defaultNaNF32UI;
        return uZ.f;
    }
    uint32_t exp = ((val >> 23) % (1 << 8));
    bool sign = (val >> 31) != 0;
    //bool sign_exp = (exp >= 127);

    // val < -126.0
    if (val > 0xc2fc0000) {
        uZ.ui = 0;
        return uZ.f;
    }

    // val >= 128.0
    if (val >= 0x43000000 && val < 0x80000000) {
        uZ.ui = infinityF32UI;
        return uZ.f;
    }

    ////printf("Sign: %d\n", sign);
    ////printf("Exp: 0x%08x\tSign: %d\n", exp, sign_exp);

    uint32_t in_integer = 0;

    if(exp <= 0x86 && exp >= 0x7f) {
        int shift = (exp - 0x7f);
        ////printf("Shift: %d\n", shift);
        in_integer =((val % (1 << 23)) >> (23-shift)) + (1 << (shift));
    }

    ////printf("IN_INTEGER: 0x%08x\n", in_integer);

    uint32_t in_shifted = 0;

    if(exp <= 0x86 && exp >= 0x66){
        int shift = (exp - 0x7d);
        if(shift > 0){
            in_shifted = (((1 << 23) + (val % (1 << 23))) << (shift)) % ( 1 << 25 );
        } else {
            in_shifted = (((1 << 23) + (val % (1 << 23))) >> (-shift)) % ( 1 << 25 );
        }
    }

      //used to be rm=RMM, but RTL is implementing rm=RTZ
    //uint32_t in_rev = ((in_shifted >> 1) + (in_shifted % 2)) % (1 << 24);
    uint32_t in_rev = (in_shifted >> 1) % (1 << 24);

    if(sign) {
        in_rev = ((~in_rev) + 1) % (1 << 24);
    }

    //printf("IN_REV: 0x%08x\n", in_rev);
    ////printf("IN_REV: 0x%08x\n", in_rev);

    bool or_mantissa = in_rev != 0;
    bool g8 = ((val >> 26) % (1 << 4)) != 0;
    bool g7 = g8 || (((val >> 23) % (1 << 3)) == 0x7);
    bool e7 = ((val >> 23) % (1 << 7)) == 0x6;
    bool g6 = g8 || (((val >> 24) % (1 << 2)) == 0x3);
    bool e6 = ((val >> 23) % (1 << 7)) == 0x5;
    bool in30 = ((val >> 30) % 2) == 1;

    ////printf("OR_MANTISSA: %d\n", or_mantissa);
    ////printf("g8: %d\tg7: %d\te7: %d\tg6: %d\te6: %d\n", g8, g7, e7, g6, e6);

    uint32_t in22_16 = ((val >> 16) % (1 << 7));

    bool in_Inf = (!sign) && in30 && g6;
    bool in_Zero = or_mantissa ? (sign && in30 && (g7 || (e7 && (in22_16 >= 0x15))))
        : (sign && in30 && (g7 || (e7 && (in22_16 >= 0x16))));
    bool in_Denorm = or_mantissa? (sign && in30 && (g6 || (e6 && (in22_16 >= 0x7e))))
        : (sign && in30 && (g6 || (e6 && (in22_16 >= 0x7f))));

    ////printf("Inf: %d\tZero: %d\tDenorm: %d\n", in_Inf, in_Zero, in_Denorm);

    uint32_t x2 = in_rev % (1 << 18);
    uint32_t idx = (in_rev >> 18) % ( 1 << 6);
    //printf("IDX: 0x%08x\n", idx);
    //printf("X2: 0x%08x\n", x2);

    uint64_t c2 = texp[idx][0];
    uint64_t c1 = texp[idx][1] * (1 << 19);
    uint64_t c0 = ( ((uint64_t)texp[idx][2]) << 29); // Previously shifted 19, adjusted to 28 to match RTL
    //printf("C2: 0x%016lx\tC1: 0x%016lx\tC0: 0x%016lx\n", c2, c1, c0);

    uint64_t mul1 = c2*x2;
    uint64_t fma1 = mul1 + c1 + (1 << 5); //TODO: check rounding at bit 5, used to be at bit 14
    //printf("MUL1: 0x%016lx\tFMA1: 0x%016lx\n", mul1, fma1);

    uint64_t fma1_part = ((fma1 >> 5) % (((uint64_t)1) << 32)); //Previously shifted 15 & cut 23, adjusted to 6 & 32 to match RTL

    uint64_t mul2 = fma1_part*x2; //Previously shifted 15 & cut 23, adjusted to 6 & 32 to match RTL
    uint64_t fma2 = mul2 + c0;
    //printf("FMA1_part: 0x%016lx\n", fma1_part);
    //printf("MUL2: 0x%016lx\tFMA2: 0x%016lx\n", mul2, fma2);

    uint32_t full_result = (fma2 >> 31) % (1 << 24); //Previously shifted 21, adjusted to 31 to match RTL
    if(in_Denorm && (in_integer >= 126) && (in_integer <= 148)){
        full_result = ((fma2 >> ((in_integer - 125) + 31)) + (1 << (24 - (in_integer - 125)))) % (1 << 24); //Previously +21, adjusted to 31 to match RTL
    }
    //printf("FULL_RESULT: 0x%08x\n", full_result);

    if (in_Inf) {
        uZ.ui = 0xff << 23;
    } else if (in_Zero || in_Denorm) {
        if (!in_Zero) {
            if (or_mantissa)
                uZ.ui = ((full_result >> 1) + (full_result % 2)) % (1 << 23);
            else
                uZ.ui = full_result % (1 << 23);
        } else {
            uZ.ui = 0;
        }
    } else {
        uint32_t out_exp = (sign? (0x7f - in_integer - or_mantissa) : (0x7f + in_integer)) % (1 << 8);
        ////printf("OUT_EXP: 0x%08x\n", out_exp);

        uZ.ui = (out_exp << 23) + (((full_result >> 1) + (full_result % 2)) % (1 << 23));
    }
#ifdef SOFTFLOAT_DENORMALS_TO_ZERO
    if (isSubnormalF32UI(uZ.ui)) {
        softfloat_raiseFlags(
            softfloat_flag_underflow | softfloat_flag_inexact);
        uZ.ui = softfloat_zeroExpSigF32UI(uZ.ui);
    }
#endif
    return uZ.f;
}


#define get(v, s, e) (((v) >> (s)) % (((uint64_t)1) << ((e) - (s) + 1)))

static uint32_t ttrans_sin_convert(uint32_t ux)
{
    float x = fpu::FLT(ux);
    double filler;
    float y = (float)modf(x, &filler);

    if(y < 0 && y <= -0.5 && y <= -0.75) {
        ////printf("-IV Quartile\n");
        y = y+1;
    }
    else if(y < 0 && y <= -0.5){
        ////printf("-III Quartile\n");
        y = -0.5-y;
    }
    else if(y < 0 && y <= -0.25){
        ////printf("-II Quartile\n");
        y = -0.5-y;
    } else if(y < 0){
        ////printf("-I Quartile\n");
    }
    else if(y > 0 && y >= 0.5 && y >= 0.75) {
        ////printf("IV Quartile\n");
        y = y-1;
    }
    else if(y > 0 && y >= 0.5) {
        ////printf("III Quartile\n");
        y = 0.5-y;
    }
    else if(y > 0 && y >= 0.25){
        ////printf("II Quartile\n");
        y = 0.5-y;
    }
    else {
        ////printf("I Quartile\n");
    }

    ////printf("[SIN TRANS] O: 0x%08x (%.10f)\tR: 0x%08x (%.10f)\n", *((uint32_t*)&x), x, *((uint32_t*)&y), y);
    return fpu::F2UI32(y);
}


float32_t f32_sin2pi(float32_t a)
{
    ui32_f32 uZ;
    uint32_t val = fpu::UI32(a);
#ifdef SOFTFLOAT_DENORMALS_TO_ZERO
    if (isSubnormalF32UI(val)) {
        softfloat_raiseFlags(softfloat_flag_denormal);
        val = softfloat_zeroExpSigF32UI(val);
    }
#endif

    switch (val) {
    case minusZeroF32UI:
    case zeroF32UI:
        uZ.ui = val;
        return uZ.f;
    case infinityF32UI:
    case minusInfinityF32UI:
        softfloat_raiseFlags(softfloat_flag_invalid);
        uZ.ui = defaultNaNF32UI;
        return uZ.f;
    }
    if (isNaNF32UI(val)) {
        if (softfloat_isSigNaNF32UI(val))
            softfloat_raiseFlags(softfloat_flag_invalid);
        uZ.ui = defaultNaNF32UI;
        return uZ.f;
    }
    //printf("VAL: 0x%08x\t\n", val);

    val = ttrans_sin_convert(val);
    //printf("VAL: 0x%08x\n", val);

    switch (val) {
    case 0xbe800000: uZ.ui = minusOneF32UI; return uZ.f;
    case 0x00000000: uZ.ui = zeroF32UI; return uZ.f;
    case 0x80000000: uZ.ui = minusZeroF32UI; return uZ.f;
    case 0x3e800000: uZ.ui = oneF32UI; return uZ.f;
    default: break;
    }

    //bool ex0 = get(val, 23, 25) == 0x7;
    bool ex1 = get(val, 23, 25) == 0x6;
    bool ex2 = get(val, 23, 25) == 0x5;
    bool ex3 = get(val, 23, 25) == 0x4;
    bool ex4 = get(val, 23, 25) == 0x3;
    bool ex5 = get(val, 23, 25) == 0x2;
    bool ex6 = get(val, 23, 25) == 0x1;
    bool ex7 = get(val, 23, 25) == 0x0;
    ////printf("Ex: %d %d %d %d %d %d %d %d\n", ex0, ex1, ex2, ex3, ex4, ex5, ex6, ex7);

    bool exq = get(val, 26, 29) == 0xf;
    bool ext2 = get(val, 26, 29) == 0xe;
    bool ext1 = get(val, 26, 29) != 0xf;
    ////printf("Exq: %d\tExt1: %d\tExt2: %d\n", exq, ext1, ext2);

    bool b0 = exq && ex1;
    bool b1 = (exq && ex2) || (b0 && get(val, 22, 22));
    ////printf("B0: %d\tB1: %d\n", b0, b1);

    uint32_t rev_in = get(val, 0, 22);
    if(b1) rev_in = get((~val+1), 0, 22);
    ////printf("REV_IN: 0x%08x\n", rev_in);

    bool fast = ext2 || ext1;
    bool normal = exq;

    bool fast1 = exq && ex1 && !(get(rev_in, 17, 21) != 0x0);
    bool fast2 = exq && ex2 && !(get(rev_in, 18, 22) != 0x0);

    bool f_appr = fast | fast1 | fast2;
    //printf("Fast: %d %d %d\tNormal: %d\tF_APPR: %d\n", fast, fast1, fast2, normal, f_appr);

    bool exin1_3 = ex1 && (get(rev_in, 21, 21) == 1);
    bool exin1_4 = ex1 && (get(rev_in, 21, 21) == 0) && (get(rev_in, 20, 20) == 1);
    bool exin1_5 = ex1 && (get(rev_in, 20, 21) == 0) && (get(rev_in, 19, 19) == 1);
    bool exin1_6 = ex1 && (get(rev_in, 19, 21) == 0) && (get(rev_in, 18, 18) == 1);
    bool exin1_7 = ex1 && (get(rev_in, 18, 21) == 0) && (get(rev_in, 17, 17) == 1);
    bool exin2_3 = ex2 && (get(rev_in, 22, 22) == 1);
    bool exin2_4 = ex2 && (get(rev_in, 22, 22) == 0) && (get(rev_in, 21, 21) == 1);
    bool exin2_5 = ex2 && (get(rev_in, 21, 22) == 0) && (get(rev_in, 20, 20) == 1);
    bool exin2_6 = ex2 && (get(rev_in, 20, 22) == 0) && (get(rev_in, 19, 19) == 1);
    bool exin2_7 = ex2 && (get(rev_in, 19, 22) == 0) && (get(rev_in, 18, 18) == 1);

    uint32_t x2 = 0;

    if(normal){
        if(ex7) x2 = get(val, 0, 22);
        else if(ex6) x2 = get(val, 0, 21) << 1;
        else if(ex5) x2 = get(val, 0, 19) << 3;
        else if(ex4) x2 = get(val, 0, 18) << 4;
        else if(ex3) x2 = get(val, 0, 17) << 5;
        else if(exin1_7) x2 = get(rev_in, 0, 16) << 6;
        else if(exin1_6) x2 = get(rev_in, 0, 16) << 6;
        else if(exin1_5) x2 = get(rev_in, 0, 15) << 7;
        else if(exin1_4) x2 = get(rev_in, 0, 15) << 7;
        else if(exin1_3) x2 = get(rev_in, 0, 15) << 7;
        else if(exin2_7) x2 = get(rev_in, 0, 17) << 5;
        else if(exin2_6) x2 = get(rev_in, 0, 17) << 5;
        else if(exin2_5) x2 = get(rev_in, 0, 16) << 6;
        else if(exin2_4) x2 = get(rev_in, 0, 16) << 6;
        else if(exin2_3) x2 = get(rev_in, 0, 16) << 6;
    }
    //printf("X2: 0x%016lx\n", x2);

    uint32_t idx = 0;

    if(normal){
        if(ex7) idx = 0;
        else if(ex6) idx = (1 << 1) + get(val, 22, 22);
        else if(ex5) idx = (1 << 3) + get(val, 20, 22);
        else if(ex4) idx = (1 << 4) + get(val, 19, 22);
        else if(ex3) idx = (1 << 5) + get(val, 18, 22);
        else if(exin1_7) idx = 0;
        else if(exin1_6) idx = (1 << 1) + get(rev_in, 17, 17);
        else if(exin1_5) idx = (1 << 3) + get(rev_in, 16, 18);
        else if(exin1_4) idx = (1 << 4) + get(rev_in, 16, 19);
        else if(exin1_3) idx = (1 << 5) + get(rev_in, 16, 20);
        else if(exin2_7) idx = 0;
        else if(exin2_6) idx = (1 << 1) + get(rev_in, 18, 18);
        else if(exin2_5) idx = (1 << 3) + get(rev_in, 17, 19);
        else if(exin2_4) idx = (1 << 4) + get(rev_in, 17, 20);
        else if(exin2_3) idx = (1 << 5) + get(rev_in, 17, 21);
    }
    //printf("IDX: 0x%016lx\n", idx);

    uint64_t c2 =  tsin[idx][0];
    uint64_t c1 = ((uint64_t)tsin[idx][1]) * (1 << 22);
    uint64_t c0 = ((uint64_t)tsin[idx][2]) * (((uint64_t)1) << 31);
    //printf("C2: 0x%016lx\tC1: 0x%016lx\tC0: 0x%016lx\n", c2, c1, c0);

    uint64_t mul1 = c2*x2;
    //printf("MUL1: 0x%016lx\n", mul1);
    uint64_t fma1 = mul1 + c1 + (1 << 12);
    //printf("FMA1: 0x%08x\n", get(fma1, 13, 42));
    uint64_t mul2 = (~get(fma1, 13, 42)+1)*x2;
    //printf("IN2: 0x%08x\tMUL2: 0x%016lx\n", (~get(fma1, 13, 42)+1), mul2);

    uint64_t fma2 = mul2 + c0 + (1 << 23);
    //printf("FMA2: 0x%016lx\n", fma2);

    uint64_t filtered_fma2 = get((get(fma2, 33, 57) + 1), 1, 24) << 10;
    //printf("FMA2 FILTERED: 0x%08x\n", (filtered_fma2>>10) );

    uint64_t norm_result = (((uint64_t)1 << 34) + filtered_fma2) * ((1 << 23) + get(rev_in, 0, 22));

    if ( exin1_3 ) norm_result = (((uint64_t)1 << 34) + filtered_fma2) * ((1 << 23) + (get(rev_in, 0, 20) << 2));
    else if( exin1_4 ) norm_result = (((uint64_t)1 << 34) + filtered_fma2) * ((1 << 23) + (get(rev_in, 0, 19) << 3));
    else if( exin1_5 ) norm_result = (((uint64_t)1 << 34) + filtered_fma2) * ((1 << 23) + (get(rev_in, 0, 18) << 4));
    else if( exin1_6 ) norm_result = (((uint64_t)1 << 34) + filtered_fma2) * ((1 << 23) + (get(rev_in, 0, 17) << 5));
    else if( exin1_7 ) norm_result = (((uint64_t)1 << 34) + filtered_fma2) * ((1 << 23) + (get(rev_in, 0, 16) << 6));
    else if( exin2_3 ) norm_result = (((uint64_t)1 << 34) + filtered_fma2) * ((1 << 23) + (get(rev_in, 0, 21) << 1));
    else if( exin2_4 ) norm_result = (((uint64_t)1 << 34) + filtered_fma2) * ((1 << 23) + (get(rev_in, 0, 20) << 2));
    else if( exin2_5 ) norm_result = (((uint64_t)1 << 34) + filtered_fma2) * ((1 << 23) + (get(rev_in, 0, 19) << 3));
    else if( exin2_6 ) norm_result = (((uint64_t)1 << 34) + filtered_fma2) * ((1 << 23) + (get(rev_in, 0, 18) << 4));
    else if( exin2_7 ) norm_result = (((uint64_t)1 << 34) + filtered_fma2) * ((1 << 23) + (get(rev_in, 0, 17) << 5));
    //printf("NORM_RESULT: 0x%016lx\n", norm_result);

    uint32_t norm_exp = get(val, 23, 30);

    if(exin1_3) norm_exp = 0x7c;
    else if(exin1_4) norm_exp = 0x7b;
    else if(exin1_5) norm_exp = 0x7a;
    else if(exin1_6) norm_exp = 0x79;
    else if(exin1_7) norm_exp = 0x78;
    else if(exin2_3) norm_exp = 0x7c;
    else if(exin2_4) norm_exp = 0x7b;
    else if(exin2_5) norm_exp = 0x7a;
    else if(exin2_6) norm_exp = 0x79;
    else if(exin2_7) norm_exp = 0x78;
    ////printf("NORM_EXP: 0x%016lx\n", norm_exp);

    uint64_t x = get(rev_in, 0, 22);

    if(fast1) {
        if(get(rev_in, 16, 16) == 1) x = (get(rev_in, 0, 15) << 7);
        else if(get(rev_in, 15, 15) == 1) x = (get(rev_in, 0, 14) << 8);
        else if(get(rev_in, 14, 14) == 1) x = (get(rev_in, 0, 13) << 9);
        else if(get(rev_in, 13, 13) == 1) x = (get(rev_in, 0, 12) << 10);
        else if(get(rev_in, 12, 12) == 1) x = (get(rev_in, 0, 11) << 11);
        else if(get(rev_in, 11, 11) == 1) x = (get(rev_in, 0, 10) << 12);
        else if(get(rev_in, 10, 10) == 1) x = (get(rev_in, 0, 9) << 13);
        else if(get(rev_in, 9, 9) == 1) x = (get(rev_in, 0, 8) << 14);
        else if(get(rev_in, 8, 8) == 1) x = (get(rev_in, 0, 7) << 15);
        else if(get(rev_in, 7, 7) == 1) x = (get(rev_in, 0, 6) << 16);
        else if(get(rev_in, 6, 6) == 1) x = (get(rev_in, 0, 5) << 17);
        else if(get(rev_in, 5, 5) == 1) x = (get(rev_in, 0, 4) << 18);
        else if(get(rev_in, 4, 4) == 1) x = (get(rev_in, 0, 3) << 19);
        else if(get(rev_in, 3, 3) == 1) x = (get(rev_in, 0, 2) << 20);
        else if(get(rev_in, 2, 2) == 1) x = (get(rev_in, 0, 1) << 21);
        else if(get(rev_in, 1, 1) == 1) x = (get(rev_in, 0, 0) << 22);
    } else if(fast2){
        if(get(rev_in, 17, 17) == 1) x = (get(rev_in, 0, 16) << 6);
        else if(get(rev_in, 16, 16) == 1) x = (get(rev_in, 0, 15) << 7);
        else if(get(rev_in, 15, 15) == 1) x = (get(rev_in, 0, 14) << 8);
        else if(get(rev_in, 14, 14) == 1) x = (get(rev_in, 0, 13) << 9);
        else if(get(rev_in, 13, 13) == 1) x = (get(rev_in, 0, 12) << 10);
        else if(get(rev_in, 12, 12) == 1) x = (get(rev_in, 0, 11) << 11);
        else if(get(rev_in, 11, 11) == 1) x = (get(rev_in, 0, 10) << 12);
        else if(get(rev_in, 10, 10) == 1) x = (get(rev_in, 0, 9) << 13);
        else if(get(rev_in, 9, 9) == 1) x = (get(rev_in, 0, 8) << 14);
        else if(get(rev_in, 8, 8) == 1) x = (get(rev_in, 0, 7) << 15);
        else if(get(rev_in, 7, 7) == 1) x = (get(rev_in, 0, 6) << 16);
        else if(get(rev_in, 6, 6) == 1) x = (get(rev_in, 0, 5) << 17);
        else if(get(rev_in, 5, 5) == 1) x = (get(rev_in, 0, 4) << 18);
        else if(get(rev_in, 4, 4) == 1) x = (get(rev_in, 0, 3) << 19);
        else if(get(rev_in, 3, 3) == 1) x = (get(rev_in, 0, 2) << 20);
        else if(get(rev_in, 2, 2) == 1) x = (get(rev_in, 0, 1) << 21);
        else if(get(rev_in, 1, 1) == 1) x = (get(rev_in, 0, 0) << 22);
        else x = 0;
    }
    //printf("X: 0x%016lx\n", x);

    uint32_t in_ex = get(val, 23, 30);
    if(fast1){
        if(get(rev_in,16,16) == 1) in_ex = 0x77;
        else if(get(rev_in,15,15) == 1) in_ex = 0x76;
        else if(get(rev_in,14,14) == 1) in_ex = 0x75;
        else if(get(rev_in,13,13) == 1) in_ex = 0x74;
        else if(get(rev_in,12,12) == 1) in_ex = 0x73;
        else if(get(rev_in,11,11) == 1) in_ex = 0x72;
        else if(get(rev_in,10,10) == 1) in_ex = 0x71;
        else if(get(rev_in,9,9) == 1) in_ex = 0x70;
        else if(get(rev_in,8,8) == 1) in_ex = 0x6f;
        else if(get(rev_in,7,7) == 1) in_ex = 0x6e;
        else if(get(rev_in,6,6) == 1) in_ex = 0x6d;
        else if(get(rev_in,5,5) == 1) in_ex = 0x6c;
        else if(get(rev_in,4,4) == 1) in_ex = 0x6b;
        else if(get(rev_in,3,3) == 1) in_ex = 0x6a;
        else if(get(rev_in,2,2) == 1) in_ex = 0x69;
        else if(get(rev_in,1,1) == 1) in_ex = 0x68;
        else in_ex = 0x67;
    } else if(fast2){
        if(get(rev_in,17,17) == 1) in_ex = 0x77;
        else if(get(rev_in,16,16) == 1) in_ex = 0x76;
        else if(get(rev_in,15,15) == 1) in_ex = 0x75;
        else if(get(rev_in,14,14) == 1) in_ex = 0x74;
        else if(get(rev_in,13,13) == 1) in_ex = 0x73;
        else if(get(rev_in,12,12) == 1) in_ex = 0x72;
        else if(get(rev_in,11,11) == 1) in_ex = 0x71;
        else if(get(rev_in,10,10) == 1) in_ex = 0x70;
        else if(get(rev_in,9,9) == 1) in_ex = 0x6f;
        else if(get(rev_in,8,8) == 1) in_ex = 0x6e;
        else if(get(rev_in,7,7) == 1) in_ex = 0x6d;
        else if(get(rev_in,6,6) == 1) in_ex = 0x6c;
        else if(get(rev_in,5,5) == 1) in_ex = 0x6b;
        else if(get(rev_in,4,4) == 1) in_ex = 0x6a;
        else if(get(rev_in,3,3) == 1) in_ex = 0x69;
        else if(get(rev_in,2,2) == 1) in_ex = 0x68;
        else if(get(rev_in,1,1) == 1) in_ex = 0x67;
        else in_ex = 0x66;
    }
    //printf("IN_EX: 0x%08x\n", in_ex);

    uint64_t in_squared = ((uint64_t)((1 << 23) + get(x, 0, 22)))*((uint64_t)((1 << 23) + get(x, 0, 22)));
    //printf("IN_SQUARED: 0x%016lx\n", in_squared);

    uint64_t twopi = 0xc90fdb00000000;
    uint64_t piCube = 0x52af;

    uint64_t interm = twopi;
    if(ext2) {
        if(get(val, 23, 25) == 0x7) interm = twopi - (piCube << 14) * get(in_squared, 32, 47);
        else if(ex1) interm = twopi - (piCube << 12) * get(in_squared, 32, 47);
        else if(ex2) interm = twopi - (piCube << 10) * get(in_squared, 32, 47);
        else if(ex3) interm = twopi - (piCube << 8) * get(in_squared, 32, 47);
        else if(ex4) interm = twopi - (piCube << 6) * get(in_squared, 32, 47);
        else if(ex5) interm = twopi - (piCube << 4) * get(in_squared, 32, 47);
        else if(ex6) interm = twopi - (piCube << 2) * get(in_squared, 32, 47);
        else if(ex7) interm = twopi - (piCube << 0) * get(in_squared, 32, 47);
    } else if (fast1) {
        if(get(rev_in, 16, 16) == 1) interm = twopi - (piCube << 14) * get(in_squared, 32, 47);
        else if(get(rev_in, 15, 15) == 1) interm = twopi - (piCube << 12) * get(in_squared, 32, 47);
        else if(get(rev_in, 14, 14) == 1) interm = twopi - (piCube << 10) * get(in_squared, 32, 47);
        else if(get(rev_in, 13, 13) == 1) interm = twopi - (piCube << 8) * get(in_squared, 32, 47);
        else if(get(rev_in, 12, 12) == 1) interm = twopi - (piCube << 6) * get(in_squared, 32, 47);
        else if(get(rev_in, 11, 11) == 1) interm = twopi - (piCube << 4) * get(in_squared, 32, 47);
        else if(get(rev_in, 10, 10) == 1) interm = twopi - (piCube << 2) * get(in_squared, 32, 47);
        else if(get(rev_in, 9, 9) == 1) interm = twopi - (piCube << 0) * get(in_squared, 32, 47);
    } else if(fast2) {
        if(get(rev_in, 17, 17) == 1) interm = twopi - (piCube << 14) * get(in_squared, 32, 47);
        else if(get(rev_in, 16, 16) == 1) interm = twopi - (piCube << 12) * get(in_squared, 32, 47);
        else if(get(rev_in, 15, 15) == 1) interm = twopi - (piCube << 10) * get(in_squared, 32, 47);
        else if(get(rev_in, 14, 14) == 1) interm = twopi - (piCube << 8) * get(in_squared, 32, 47);
        else if(get(rev_in, 13, 13) == 1) interm = twopi - (piCube << 6) * get(in_squared, 32, 47);
        else if(get(rev_in, 12, 12) == 1) interm = twopi - (piCube << 4) * get(in_squared, 32, 47);
        else interm = twopi - (piCube << 2) * get(in_squared, 32, 47);
    }
    //printf("INTERM: 0x%016lx\n", interm);

    uint64_t fast_result = get(interm, 29, 55) * ((1<<23) + get(x, 0, 22));
    //printf("FAST_RESULT: 0x%016lx\n", fast_result);

    uint32_t out_s = get(val, 31, 31) ^ b0;

    uint32_t out_e = get(norm_exp, 0, 7) + (1 << 1);

    if(fast && (get(fast_result, 50, 50) == 1)) out_e = get(val, 23, 30) + 3;
    else if(fast && (get(fast_result, 50, 50) == 0)) out_e = get(val, 23, 30) + 2;
    else if(fast1 && (get(fast_result, 50, 50) == 1)) out_e = get(in_ex, 0, 7) + 3;
    else if(fast1 && (get(fast_result, 50, 50) == 0)) out_e = get(in_ex, 0, 7) + 2;
    else if(fast2 && (get(fast_result, 50, 50) == 1)) out_e = get(in_ex, 0, 7) + 3;
    else if(fast2 && (get(fast_result, 50, 50) == 0)) out_e = get(in_ex, 0, 7) + 2;
    else if(get(norm_result + (((uint64_t)1)<<34), 58, 58) == 1) out_e = get(norm_exp, 0, 7) + 3;
    else if(get(norm_result, 58, 58) == 1) out_e = get(norm_exp, 0, 7) + 3;
    ////printf("OUT_S: 0x%08x\tOUT_E: 0x%08x\n", out_s, out_e);

    uint32_t out_m = get(norm_result, 33, 56) + 1;

    if(f_appr) {
        if(get(fast_result, 50, 50) == 1) out_m = get(fast_result, 26, 49) + 1;
        else if(get(fast_result, 50, 50) == 0) {
            out_m = get(fast_result, 25, 48) + 1;
            if(get(out_m, 24, 24)) {
                // FIX ARCHSIM-379: unexpected output mantissa overflow
                out_m = get(fast_result, 26, 49) + 1;
                out_e = get(val, 23, 30) + 3;
            }
        }
    } else if (get(norm_result + (((uint64_t) 1 )<<34), 58, 58) == 1) out_m = get(norm_result, 34, 57) + 1;
    else if (get(norm_result, 58, 58) == 1) out_m = get(norm_result, 34, 57) + 1;
    ////printf("OUT_M: 0x%08x\n", out_m);

    uZ.ui = (get(out_s, 0, 0) << 31) + (get(out_e, 0, 7) << 23) + get(out_m, 1, 23);
#ifdef SOFTFLOAT_DENORMALS_TO_ZERO
    if (isSubnormalF32UI(uZ.ui)) {
        softfloat_raiseFlags(
            softfloat_flag_underflow | softfloat_flag_inexact);
        uZ.ui = softfloat_zeroExpSigF32UI(uZ.ui);
    }
#endif
    return uZ.f;
}
