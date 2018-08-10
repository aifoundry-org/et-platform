#ifndef _EMU_CASTS_H
#define _EMU_CASTS_H

#include "emu_defines.h"

static inline uint32_t cast_float32_to_uint32(float32_t val)
{
    iufval tmp;
    tmp.f = val;
    return tmp.u;
}

static inline float32_t cast_uint32_to_float32(uint32_t val)
{
    iufval tmp;
    tmp.u = val;
    return tmp.f;
}

#ifdef USE_REAL_TXFMA
static inline uint64_t cast_double_to_uint64(double val)
{
    iufval tmp;
    tmp.dbl = val;
    return tmp.x;
}

static inline double cast_uint64_to_double(uint64_t val)
{
    iufval tmp;
    tmp.x = val;
    return tmp.dbl;
}

static inline float cast_uint32_to_float(uint32_t val)
{
    iufval tmp;
    tmp.u = val;
    return tmp.flt;
}
#endif

static inline float32_t cast_float_to_float32(float val)
{
    iufval tmp;
    tmp.flt = val;
    return tmp.f;
}

static inline float cast_float32_to_float(float32_t val)
{
    iufval tmp;
    tmp.f = val;
    return tmp.flt;
}

#endif // _EMU_CASTS_H
