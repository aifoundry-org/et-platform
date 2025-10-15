/*-------------------------------------------------------------------------
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------*/

#include <bitset>
#include <iostream>
#include <iomanip>
#include <cstring>

#include "softfloat/platform.h"
#include "softfloat/internals.h"
#include "softfloat/specialize.h"
#include "softfloat/softfloat.h"


struct Float16 {
    bool     sign;
    int8_t   exp;
    uint16_t sig;

    Float16(bool s, int16_t e, uint32_t m)
        : sign(s), exp(e), sig(m)
        {}

    Float16(float16_t x)
        : sign(signF16UI(x.v)), exp(expF16UI(x.v)), sig(fracF16UI(x.v))
        {}
};


template<size_t N=7>
struct Float32 {
    bool     sign;
    int16_t  exp;
    uint64_t sig;

    Float32(bool s, int16_t e, uint64_t m)
        : sign(s), exp(e), sig(m)
        {}

    Float32(float32_t x)
        : sign(signF32UI(x.v)), exp(expF32UI(x.v)), sig(fracF32UI(x.v))
        {}
};


struct FFlags {
    uint8_t val;
    FFlags(uint8_t x) : val(x) {}
};

#define SOFTFLOAT_FLAGS FFlags(softfloat_exceptionFlags)


struct RMode {
    uint8_t val;
    RMode(uint8_t x) : val(x) {};
};


static inline float u2f(uint32_t x)
{
    union { float f; uint32_t u; } t;
    t.u = x;
    return t.f;
}


static inline std::ostream& operator<<(std::ostream& os, Float16 x)
{
    std::bitset<1>  sign(x.sign);
    std::bitset<5>  exp(x.exp);
    std::bitset<5>  ovfl(x.sig >> 11);
    std::bitset<1>  lead(x.sig >> 10);
    std::bitset<10> frac(x.sig);
    return os << '(' << sign << ' ' << exp << ' ' << ovfl << ' ' << lead << ' ' << frac << ')';
}


static inline std::ostream& operator<<(std::ostream& os, Float32<0> x)
{
    std::bitset<1>  sign(x.sign);
    std::bitset<8>  exp(x.exp);
    std::bitset<9>  ovfl(x.sig >> 24);
    std::bitset<1>  lead(x.sig >> 23);
    std::bitset<23> frac(x.sig);
    return os << '(' << sign << ' ' << exp << ' ' << ovfl << ' ' << lead << ' ' << frac << ')';
}


static inline std::ostream& operator<<(std::ostream& os, Float32<1> x)
{
    std::bitset<1>  sign(x.sign);
    std::bitset<8>  exp(x.exp);
    std::bitset<8>  ovfl(x.sig >> 25);
    std::bitset<1>  lead(x.sig >> 24);
    std::bitset<23> frac(x.sig >>  1);
    std::bitset<1>  grd(x.sig);
    return os << '(' << sign << ' ' << exp << ' ' << ovfl << ' ' << lead << ' ' << frac << ' ' << grd << ')';
}


static inline std::ostream& operator<<(std::ostream& os, Float32<2> x)
{
    std::bitset<1>  sign(x.sign);
    std::bitset<8>  exp(x.exp);
    std::bitset<7>  ovfl(x.sig >> 26);
    std::bitset<1>  lead(x.sig >> 25);
    std::bitset<23> frac(x.sig >>  2);
    std::bitset<1>  grd(x.sig >> 1);
    std::bitset<1>  rnd(x.sig);
    return os << '(' << sign << ' ' << exp << ' ' << ovfl << ' ' << lead << ' ' << frac << ' ' << grd << ' ' << rnd << ')';
}


static inline std::ostream& operator<<(std::ostream& os, Float32<3> x)
{
    std::bitset<1>  sign(x.sign);
    std::bitset<8>  exp(x.exp);
    std::bitset<6>  ovfl(x.sig >> 27);
    std::bitset<1>  lead(x.sig >> 26);
    std::bitset<23> frac(x.sig >>  3);
    std::bitset<1>  grd(x.sig >> 2);
    std::bitset<2>  rnd(x.sig);
    return os << '(' << sign << ' ' << exp << ' ' << ovfl << ' ' << lead << ' ' << frac << ' ' << grd << ' ' << rnd << ')';
}


template<size_t N>
static inline std::ostream& operator<<(std::ostream& os, Float32<N> x)
{
    std::bitset<1>    sign(x.sign);
    std::bitset<8>    exp(x.exp);
    std::bitset<9-N>  ovfl(x.sig >> (N+24));
    std::bitset<1>    lead(x.sig >> (N+23));
    std::bitset<23>   frac(x.sig >> N);
    std::bitset<1>    grd(x.sig >> (N-1));
    std::bitset<N-3>  rnd(x.sig >> 2);
    std::bitset<2>    stck(x.sig);
    return os
        << "0x" << std::hex << std::setw(8) << std::setfill('0') << x.sig << std::dec
        << " (" << sign << ' ' << exp << ' ' << ovfl << ' ' << lead << ' ' << frac << ' ' << grd << ' ' << rnd << " s:" << stck << ')';
}


static inline std::ostream& operator<<(std::ostream& os, float16_t x)
{
    std::bitset<1> sign(signF16UI(x.v));
    std::bitset<5> exp(expF16UI(x.v));
    std::bitset<10> frac(fracF16UI(x.v));
    return os
        << "0x"
        << std::hex << std::setw(4) << std::setfill('0') << x.v
        << std::dec <<  " (" << sign << ' ' << exp << ' ' << frac << ')';
}


static inline std::ostream& operator<<(std::ostream& os, float32_t x)
{
    std::bitset<1> sign(signF32UI(x.v));
    std::bitset<8> exp(expF32UI(x.v));
    std::bitset<23> frac(fracF32UI(x.v));
    return os
        << "0x"
        << std::hex << std::setw(8) << std::setfill('0') << x.v
        << std::dec <<  " (" << sign << ' ' << exp << ' ' << frac << ')';
}


static inline std::ostream& operator<<(std::ostream& os, FFlags x)
{
    if (!x.val) {
        return os << "0";
    }
    bool coma = false;
    if (x.val & 0x80) {
        os << (coma ? ",ID" : "ID");
        coma = true;
    }
    if (x.val & 0x10) {
        os << (coma ? ",NV" : "NV");
        coma = true;
    }
    if (x.val & 0x08) {
        os << (coma ? ",DZ" : "DZ");
        coma = true;
    }
    if (x.val & 0x04) {
        os << (coma ? ",OF" : "OF");
        coma = true;
    }
    if (x.val & 0x02) {
        os << (coma ? ",UF" : "UF");
        coma = true;
    }
    if (x.val & 0x01) {
        os << (coma ? ",NX" : "NX");
    }
    return os;
}


static inline std::ostream& operator<<(std::ostream& os, RMode x)
{
    switch (x.val) {
        case softfloat_round_near_even  : return os << "near_even";
        case softfloat_round_minMag     : return os << "minMag";
        case softfloat_round_min        : return os << "min";
        case softfloat_round_max        : return os << "max";
        case softfloat_round_near_maxMag: return os << "near_maxMag";
    }
    return os << "unknown";
}


static inline uint8_t str2rm(const char* str)
{
    if (!strcmp(str, "rne") || !strcmp(str, "rnear_even")) {
        return softfloat_round_near_even;
    }
    if (!strcmp(str, "rtz") || !strcmp(str, "rminMag")) {
        return softfloat_round_minMag;
    }
    if (!strcmp(str, "rdn") || !strcmp(str, "rmin")) {
        return softfloat_round_min;
    }
    if (!strcmp(str, "rup") || !strcmp(str, "rmax")) {
        return softfloat_round_max;
    }
    if (!strcmp(str, "rmm") || !strcmp(str, "rnear_maxMag")) {
        return softfloat_round_near_maxMag;
    }
    return -1;
}
