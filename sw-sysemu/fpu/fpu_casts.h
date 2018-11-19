#ifndef _FPU_CASTS_H
#define _FPU_CASTS_H

#include "fpu_types.h"
#include "softfloat/internals.h"

union ui16_f11 { uint16_t ui; float11_t f; };
union ui16_f10 { uint16_t ui; float10_t f; };

namespace fpu {

    static inline float FLT(uint32_t x)
    {
        union { uint32_t ui; float flt; } uZ;
        uZ.ui = x;
        return uZ.flt;
    }


    static inline float FLT(float32_t x)
    {
        union { float32_t f; float flt; } uZ;
        uZ.f = x;
        return uZ.flt;
    }

    static inline float32_t F32(uint32_t x)
    {
        ui32_f32 uZ;
        uZ.ui = x;
        return uZ.f;
    }

    static inline float32_t F2F32(float x)
    {
        union { float32_t f; float flt; } uZ;
        uZ.flt = x;
        return uZ.f;
    }

    static inline float16_t F16(uint16_t x)
    {
        ui16_f16 uZ;
        uZ.ui = x;
        return uZ.f;
    }


    static inline float11_t F11(uint16_t x)
    {
        ui16_f11 uZ;
        uZ.ui = x;
        return uZ.f;
    }


    static inline float10_t F10(uint16_t x)
    {
        ui16_f10 uZ;
        uZ.ui = x;
        return uZ.f;
    }


    static inline uint32_t UI32(float x)
    {
        union { uint32_t ui; float flt; } uZ;
        uZ.flt = x;
        return uZ.ui;
    }


    static inline uint32_t UI32(float32_t x)
    {
        ui32_f32 uZ;
        uZ.f = x;
        return uZ.ui;
    }


    static inline uint16_t UI16(float16_t x)
    {
        ui16_f16 uZ;
        uZ.f = x;
        return uZ.ui;
    }


    static inline uint16_t UI16(float11_t x)
    {
        ui16_f11 uZ;
        uZ.f = x;
        return uZ.ui;
    }


    static inline uint16_t UI16(float10_t x)
    {
        ui16_f10 uZ;
        uZ.f = x;
        return uZ.ui;
    }

} // namespace fpu

#endif // _FPU_CASTS_H
