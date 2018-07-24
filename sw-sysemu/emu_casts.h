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

#endif // _EMU_CASTS_H
