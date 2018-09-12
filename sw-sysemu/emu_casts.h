#ifndef _EMU_CASTS_H
#define _EMU_CASTS_H

#include "fpu_types.h"

typedef union {
    int32_t  i;
    uint32_t u;
    int64_t  l;
    uint64_t lu;
} u32_i32_u64_i64;

typedef union {
    int16_t   i;
    uint16_t  u;
    float16_t f16;
    float11_t f11;
    float10_t f10;
} iufval16;

static inline float10_t cast_uint16_to_float10(uint16_t val)
{
    iufval16 tmp;
    tmp.u = val;
    return tmp.f10;
}

static inline float11_t cast_uint16_to_float11(uint16_t val)
{
    iufval16 tmp;
    tmp.u = val;
    return tmp.f11;
}

static inline float16_t cast_uint16_to_float16(uint16_t val)
{
    iufval16 tmp;
    tmp.u = val;
    return tmp.f16;
}

static inline uint_fast16_t cast_float10_to_uint16(float10_t val)
{
    iufval16 tmp;
    tmp.f10 = val;
    return tmp.u;
}

static inline uint_fast16_t cast_float11_to_uint16(float11_t val)
{
    iufval16 tmp;
    tmp.f11 = val;
    return tmp.u;
}

static inline uint_fast16_t cast_float16_to_uint16(float16_t val)
{
    iufval16 tmp;
    tmp.f16 = val;
    return tmp.u;
}

typedef union {
    int32_t   i;
    uint32_t  u;
    float32_t f;
    float     flt;
} iufval32;

static inline float32_t cast_uint32_to_float32(uint32_t val)
{
    iufval32 tmp;
    tmp.u = val;
    return tmp.f;
}

static inline float cast_uint32_to_float(uint32_t val)
{
    iufval32 tmp;
    tmp.u = val;
    return tmp.flt;
}

static inline uint32_t cast_float32_to_uint32(float32_t val)
{
    iufval32 tmp;
    tmp.f = val;
    return tmp.u;
}

static inline float cast_float32_to_float(float32_t val)
{
    iufval32 tmp;
    tmp.f = val;
    return tmp.flt;
}

static inline float32_t cast_float_to_float32(float val)
{
    iufval32 tmp;
    tmp.flt = val;
    return tmp.f;
}

#if 0//defined(USE_REAL_TXFMA)
typedef union {
    uint64_t  x;
    int64_t   xs;
    double    dbl;
} iufval64;

static inline double cast_uint64_to_double(uint64_t val)
{
    iufval64 tmp;
    tmp.x = val;
    return tmp.dbl;
}

static inline uint64_t cast_double_to_uint64(double val)
{
    iufval64 tmp;
    tmp.dbl = val;
    return tmp.x;
}
#endif

#endif // _EMU_CASTS_H
