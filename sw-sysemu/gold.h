/*-------------------------------------------------------------------------
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------*/

#ifndef BEMU_GOLD_H
#define BEMU_GOLD_H

#include <stdexcept>
#include "fpu/fpu_types.h"

namespace gld {


inline bool isNaN(uint32_t a) {
    return (((~(a) & 0x7f800000) == 0) && ((a) & 0x007fffff));
}

inline bool security_ulp_check(uint32_t a, uint32_t b)
{
    uint32_t a_exp = (a & 0x7f800000);
    uint32_t b_exp = (b & 0x7f800000);
    if (a_exp == 0x7f800000 || b_exp == 0x7f800000) {
        return a != b;
    }
    return (a != b) && ((a+1) != b) && (a != (b+1));
}

float32_t f32_rsqrt(float32_t);
float32_t f32_sin2pi(float32_t);
float32_t f32_exp2(float32_t);
float32_t f32_log2(float32_t);
float32_t f32_rcp(float32_t);

int32_t fxp1714_rcpStep(int32_t, int32_t);


} // namespace gld

#endif // BEMU_GOLD_H
