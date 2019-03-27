/* vim: set ts=8 sw=4 et sta cin cino=\:0s,l1,g0,N-s,E-s,i0,+2s,(0,W2s : */

#ifndef BEMU_GOLD_H
#define BEMU_GOLD_H

#include "fpu/fpu_types.h"

namespace gld {


inline bool isNaN(uint32_t a) {
    return (((~(a) & 0x7f800000) == 0) && ((a) & 0x007fffff));
}

float32_t f32_rsqrt(float32_t);
float32_t f32_sin2pi(float32_t);
float32_t f32_exp2(float32_t);
float32_t f32_log2(float32_t);
float32_t f32_rcp(float32_t);

int32_t fxp1714_rcpStep(int32_t, int32_t);


} // namespace gld

#endif // BEMU_GOLD_H
